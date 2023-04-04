#include "minecraft.h"
#include <chunk.h>

// PACKET
void packet::write(const uint8_t val){
    buffer[index] = val;
    index ++;
}

void packet::write(const uint8_t * buf, size_t size){
    memcpy(buffer + index, std::move(buf), size);
    index += size;
}

void packet::serverWriteVarInt(int32_t value){
    do {
        uint8_t temp = (uint8_t)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S->write(temp);
    } while (value != 0);
}

void packet::writePacket(){
    (*mtx).lock();
    serverWriteVarInt(index);
    S->write(buffer, index);
    (*mtx).unlock();
}

// SERVERBOUND LOGIN
uint8_t minecraft::player::readHandShake(){
    readVarInt(); // length
    int id = readVarInt(); // packet id
    int protocol_version = readVarInt();
    readString(); // we don't need our name
    readUnsignedShort();
    int state = readVarInt();
    /* if(id != 0){
        return false;
    } else if(protocol_version != 754){
        logerr("wrong protocol version, log in with 1.16.5");
        return false;
    } */
    // lol, just take a gamble ezpz

    if(state != 1 && state != 2) {
        logerr("wrong state");
        return 0;
    } else {
        return state;
    }
}

bool minecraft::player::readLoginStart(){
    readVarInt(); // length
    int id = readVarInt();
    if(id != 0){
        return false;
    }
    username = readString();
    loginfo("Login: " + username);
    return true;
}

uint64_t minecraft::player::readPing(){
    while(S->available() < 10);
    readVarInt(); // length
    readVarInt(); // packet id
    uint64_t payload = readLong(); // payload
    login("Ping: " + String((uint32_t)payload));
    return payload;
}

void minecraft::player::readRequest(){
    while(S->available() < 2);
    readVarInt();
    readVarInt();
    login("Request packet received");
}

// SERVERBOUND PLAY PACKETS
void minecraft::player::readChat(){
    String m = readString();
    login("<" + username + "> " + m);
    if(m == "/stats"){
        writeChat("freeheap: " + String(esp_get_free_heap_size() / 1000) + "kB", "Server");
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    } else if (m == "/on") {
        digitalWrite(26, HIGH);
        mc->broadcastChatMessage("Turned LED on", "LED");
    } else if (m == "/off") {
        digitalWrite(26, LOW);
        mc->broadcastChatMessage("Turned LED off", "LED");
    }
    else {
        mc->broadcastChatMessage(m, username);
    }
}

void minecraft::player::readClientStatus(){
    int32_t action = readVarInt();
    if (action == 0) {
        writeRespawn();
    } else {
        loginfo("Ignoring client status statistics packet");
    }
}

void minecraft::player::readClickWindow(){
    loginfo("Click window packet received");

    int8_t windowID = readByte();
    int16_t slot = readShort();
    uint8_t button = readByte();
    int16_t action = readShort();
    int32_t mode = readVarInt();
    bool present = readBool();
    loginfo("Window ID: " + String(windowID));
    loginfo("Slot: " + String(slot));
    loginfo("Button: " + String(button));
    loginfo("Action: " + String(action));
    loginfo("Mode: " + String(mode));
    loginfo("Present: " + String(present));
    if(present){
        int32_t itemID = readVarInt();
        int8_t itemCount = readByte();
        bool endOfNBT = false;
        while (!endOfNBT) {
            uint8_t nbtByte = readByte();
            if (nbtByte == 0) {
                endOfNBT = true;
            }
        }
        loginfo("Item ID: " + String(itemID));
        loginfo("Item Count: " + String(itemCount));
    }

}

void minecraft::player::readInteractEntity(){
    uint8_t entityID = readVarInt();
    uint8_t action = readVarInt();
    if (action == 3) {
        float x = readFloat();
        float y = readFloat();
        float z = readFloat();
    }
    if (action != 1) {
        uint8_t hand = readVarInt();
    }
    bool sneaking = readBool();

    if (action != 1) {  // no need to do any processing, we don't care about other actions
        return;
    }

    player* targetPlayer = &mc->players[entityID];


    double targetX = targetPlayer->x;
    double targetY = targetPlayer->y;
    double targetZ = targetPlayer->z;

    double xDiff = targetX - x;
    double yDiff = targetY - y;
    double zDiff = targetZ - z;

    double distance = sqrt(xDiff + yDiff + zDiff);

    float strengthMultiplier = 2500;
    float upVelocity = 3000;

    short xVelocity = xDiff * strengthMultiplier;
    short yVelocity = (yDiff * strengthMultiplier) + upVelocity;
    short zVelocity = zDiff * strengthMultiplier;

    if (distance > 4) {
        mc->broadcastChatMessage("he's hacking!!!", "Server");
        return;
    }

    mc->broadcastEntityAnimation(1, targetPlayer->id, true); // take damage animation
    mc->broadcastEntityStatus(2, id, true); // play hurt sound
    targetPlayer->health -= 1;
    targetPlayer->writeHealth();
    mc->broadcastEntityHealth(targetPlayer->health, targetPlayer->id);
    if (targetPlayer->health <= 0) {
        targetPlayer->death(targetPlayer->username + " was slain by " + username);
    }

    packet p(targetPlayer->S, targetPlayer->mtx);
    p.writeVarInt(0x46);
    p.writeVarInt(targetPlayer->id);
    p.writeShort(xVelocity);
    p.writeShort(yVelocity);
    p.writeShort(zVelocity);
    p.writePacket();
}

void minecraft::player::handleOnGround(bool onGround, double y) {
    bool prevOnGround = on_ground;
    on_ground = onGround;
    if (prevOnGround != on_ground) {
        loginfo("Changed on_ground to " + String(on_ground));
        if (on_ground) {
            int16_t distanceFallen = floor(fallingHeight - y);
            loginfo("Distance fallen: " + String(distanceFallen));
            if (distanceFallen > 3) {
                health -= distanceFallen - 3; // reduce health by the distance fallen
                mc->broadcastEntityStatus(2, id, true); // play hurt sound
                writeHealth(); // send health data to client
                mc->broadcastEntityHealth(health, id);
                if (health <= 0) {
                    death(username + " fell from a high place");
                }
            }
            fallingHeight = 0;
        }
    }

    if (!on_ground && y > fallingHeight) fallingHeight = y;
}

void minecraft::player::readPosition(){
    x = readDouble();
    y = readDouble();
    z = readDouble();

    if (health == 0) return; // if they are dead then don't do anything

    handleOnGround(readBool(), y);

    if (y < -100) {
        health = 0;
        food = 0;
        food_sat = 0;
        writeHealth();
        death(username + " fell into the void");
    }
    mc->broadcastPlayerPosAndLook(x, y, z, yaw_i, pitch_i, on_ground, id);
    // login("player pos " + String(x) + " " + String(y) + " " + String(z));
}

void minecraft::player::readRotation(){
    yaw = readFloat();
    pitch = readFloat();
    yaw_i = floor(fmap(yaw, 0, 360, 0, 256));
    pitch_i = floor(fmap(pitch, 0, 360, 0, 256));
    handleOnGround(readBool(), y);
    mc->broadcastPlayerRotation(yaw_i, pitch_i, on_ground, id);
    // login("player rotation " + String(yaw) + " " + String(pitch));
}

void minecraft::player::readKeepAlive(){
    login("keepalive received: " + String((long)readLong()));
}

void minecraft::player::readPositionAndLook(){
    x = readDouble();
    y = readDouble();
    z = readDouble();
    yaw = readFloat();
    pitch = readFloat();
    yaw_i = floor(fmap(yaw, 0, 360, 0, 256));
    pitch_i = floor(fmap(pitch, 0, 360, 0, 256));
    handleOnGround(readBool(), y);
    mc->broadcastPlayerPosAndLook(x, y, z, yaw_i, pitch_i, on_ground, id);
    // login("player rotation " + String(yaw) + " " + String(pitch));
}

void minecraft::player::readTeleportConfirm(){
    readVarInt();
    login("teleport confirm");
}

void minecraft::player::readHeldItem(){
    selectedSlot = readUnsignedShort() + 36;
    slot* selectedItem = &inventory[selectedSlot];

    if (selectedItem->present) {
        mc->broadcastEntityEquipment(id, 0, true, selectedItem->itemID, selectedItem->itemCount);
    } else {
        mc->broadcastEntityEquipment(id, 0, false, 0, 0);
    }
}

void minecraft::player::readAnimation(){
    uint8_t hand = readVarInt();
    uint8_t animID;
    if (hand) {
        animID = 3;
    } else {
        animID = 0;
    }
    mc->broadcastEntityAnimation(animID, id, false);  
}

void minecraft::player::readAction(){
    int32_t status = readVarInt();
    uint64_t position = readUnsignedLong();
    uint8_t face = readByte();

    int32_t blockX = position >> 38;
    int32_t blockY = position & 0xFFF;
    int32_t blockZ = (position >> 12) & 0x3FFFFFF;

    if (blockY > 16) return;

    int8_t chunkX = blockX / 16;
    int8_t chunkZ = blockZ / 16;

    if (chunkX > 1 || chunkZ > 1 || chunkX < 0 || chunkZ < 0) {
        return;
    }
    int8_t chunkBlockX = blockX % 16;
    int8_t chunkBlockZ = blockZ % 16;
    int8_t chunkBlockArrayX;
    if (chunkBlockX < 8) {
        chunkBlockArrayX = 7 - chunkBlockX;
    } else {
        chunkBlockArrayX = 15 - (chunkBlockX - 8);
    }
    u_int8_t blockAction = chunk[chunkX][chunkZ][blockY][chunkBlockZ][chunkBlockArrayX];

    switch (status) {
        case 0:
            for(auto& player : mc->players){
                if(player.connected && player.id != id){
                    // meh just show animation stage 9
                    // i'm not going to calculate how long the block will take to break and send each stage of the animation
                    player.writeBlockBreakAnimation(id, blockX, blockY, blockZ, 9);
                }
            }
            break;
        case 1:
            for(auto& player : mc->players){
                if(player.connected && player.id != id){
                    player.writeBlockBreakAnimation(id, blockX, blockY, blockZ, 10); // any number that is not 0-9 will stop the animation
                }
            }
            break;
        case 2:
            chunk[chunkX][chunkZ][blockY][chunkBlockZ][chunkBlockArrayX] = 0x00;
            mc->broadcastBlockChange(blockX, blockY, blockZ, 0x00, id);

            uint16_t itemID = blockToItem(blockAction);
            uint8_t slotToAddTo = findFreeInvSlot(itemID);
            loginfo("Adding to slot " + String(slotToAddTo));
            if (slotToAddTo != 0 && itemID != 0) {
                uint8_t currentAmount;
                if (!inventory[slotToAddTo].present) currentAmount = 0;
                else currentAmount = inventory[slotToAddTo].itemCount;
                uint8_t newAmount = currentAmount + 1;
                inventory[slotToAddTo] = {true, itemID, newAmount};
                writeInventorySlot(true, slotToAddTo, itemID, newAmount);
            }
            break;
    }
}

void minecraft::player::readEntityAction(){
    readVarInt(); // we don't need our own id lmao
    uint8_t actionID = readVarInt();
    uint8_t poseID;
    switch (actionID) {
        case 0:
            poseID = 5;
            break;
        case 1:
            poseID = 0;
            break;
    }
    mc->broadcastEntityPose(poseID, id);
    readVarInt(); // we don't need horse jump boost
}

void minecraft::player::readBlockPlacement(){
    uint8_t hand = readVarInt(); // 0: main hand, 1: off hand.
    uint64_t position = readUnsignedLong();
    uint8_t face = readVarInt();
    for (uint8_t i = 0; i < 3; i++) {
        readFloat(); // cursor position on block, not needed right now
    }
    readBool(); // if the player's head is inside a block, not needed right now

    int32_t blockX = position >> 38;
    int32_t blockY = position & 0xFFF;
    int32_t blockZ = (position >> 12) & 0x3FFFFFF;

    switch (face) {
        case 0:
            blockY--;
            break;
        case 1:
            blockY++;
            break;
        case 2:
            blockZ--;
            break;
        case 3:
            blockZ++;
            break;
        case 4:
            blockX--;
            break;
        case 5:
            blockX++;
            break;
    }

    if (blockX < 0 || blockY < 0 || blockZ < 0) return;

    int8_t chunkX = blockX / 16;
    int8_t chunkZ = blockZ / 16;

    if (chunkX > 1 || chunkZ > 1 || chunkX < 0 || chunkZ < 0) {
        return;
    }


    uint8_t slotPlacedFrom;
    if (hand == 0) slotPlacedFrom = selectedSlot; else slotPlacedFrom = 45;

    if (!inventory[slotPlacedFrom].present) {
        return;
    }

    // check if block will collide with player
    for(const auto& player : mc->players){
        if(player.connected){
            if (player.y + 1.7 >= blockY && std::floor(player.y) <= blockY && player.x + 0.3 >= blockX && player.x - 0.3 <= blockX + 1 && player.z + 0.3 >= blockZ && player.z - 0.3 <= blockZ + 1) {
                return;
            }
        }
    }

    int8_t chunkBlockX = blockX % 16;
    int8_t chunkBlockZ = blockZ % 16;

    if (blockY >= 16) {
        writeBlockChange(blockX, blockY, blockZ, 0x00);
        return;
    }

    int8_t chunkBlockArrayX;
    if (chunkBlockX < 8) {
        chunkBlockArrayX = 7 - chunkBlockX;
    } else {
        chunkBlockArrayX = 15 - (chunkBlockX - 8);
    }

    uint16_t blockPlaced = inventory[slotPlacedFrom].itemID;
    uint8_t currentItemCount = inventory[slotPlacedFrom].itemCount;
    uint8_t newItemCount = currentItemCount - 1;
    if (newItemCount == 0) {
        inventory[slotPlacedFrom].present = false;
    } else {
        inventory[slotPlacedFrom].itemCount = newItemCount;
    }
    writeInventoryItems();

    blockPlaced = itemToBlock(blockPlaced);
    if (blockPlaced == 0) blockPlaced = 1;

    chunk[chunkX][chunkZ][blockY][chunkBlockZ][chunkBlockArrayX] = blockPlaced;

    mc->broadcastBlockChange(blockX, blockY, blockZ, blockPlaced, id);
}

void minecraft::broadcastChunk(int8_t chunkX, int8_t chunkZ, uint8_t excludedPlayerID) {
    for(auto& player : players){
        if(player.connected && player.id != excludedPlayerID){
            player.writeChunk(chunkX, chunkZ);
        }
    }
}

// CLIENTBOUND BROADCAST
void minecraft::broadcastChatMessage(String msg, String username){
    for(auto& player : players){
        if(player.connected){
            player.writeChat(msg, username);
        }
    }
}

void minecraft::broadcastSpawnPlayer(uint8_t id){
    player* p = &players[id];
    for(auto& player : players){
        if(player.connected && player.id != id){
            player.writeSpawnPlayer(p->x, p->y, p->z, p->yaw_i, p->pitch_i, p->id);
            player.writeEntityLook(p->yaw_i, p->id);
            
            p->writeSpawnPlayer(player.x, player.y, player.z, player.yaw_i, player.pitch_i, player.id);
            p->writeEntityLook(player.yaw_i, player.id);
        }
    }
}

void minecraft::broadcastPlayerPosAndLook(double x, double y, double z, int _yaw_i, int _pitch_i, bool on_ground, uint8_t id){
    for(auto& player : players){
        if(player.connected && player.id != id){
            player.writeEntityTeleport(x, y, z, _yaw_i, _pitch_i, on_ground, id);
            player.writeEntityLook(_yaw_i, id);
        }
    }
}

void minecraft::broadcastPlayerRotation(int _yaw_i, int _pitch_i, bool on_ground, uint8_t id){
    for(auto& player : players){
        if(player.connected && player.id != id){
            player.writeEntityRotation(_yaw_i, _pitch_i, on_ground, id);
            player.writeEntityLook(_yaw_i, id);
        }
    }
}

void minecraft::broadcastEntityAnimation(uint8_t anim, uint8_t id, bool sendToSelf){
    for(auto& player : players){
        if(player.connected && (player.id != id || sendToSelf)){
            player.writeEntityAnimation(anim, id);
        }
    }
}

void minecraft::broadcastEntityStatus(uint8_t status, uint8_t id, bool sendToSelf){
    for(auto& player : players){
        if(player.connected && (player.id != id || sendToSelf)){
            player.writeEntityStatus(status, id);
            Serial.print("sent entity status ");
            Serial.print(status);
            Serial.print(" for id ");
            Serial.print(id);
            Serial.print(" to player ");
            Serial.print(player.username);
            Serial.print(" with id ");
            Serial.println(player.id);
        }
    }
}

void minecraft::broadcastEntityPose(uint8_t pose, uint8_t id){
    for(auto& player : players){
        if(player.connected && player.id != id){
            player.writeEntityPose(pose, id);
        }
    }
}

void minecraft::broadcastEntityHealth(float health, uint8_t id){
    for(auto& player : players){
        if(player.connected && player.id != id){
            player.writeEntityHealth(health, id);
        }
    }
}

void minecraft::broadcastEntityDestroy(uint8_t id){
    for(auto& player : players){
        if(player.connected && player.id != id){
            player.writeEntityDestroy(id);
        }
    }
}

void minecraft::broadcastEntityEquipment(uint8_t id, uint8_t slot, bool present, uint16_t itemID, uint8_t itemCount) {
    for(auto& player : players){
        if(player.connected && player.id != id){
            player.writeEntityEquipment(id, slot, present, itemID, itemCount);
        }
    }
}

void minecraft::broadcastBlockChange(int64_t x, int64_t y, int64_t z, uint16_t blockID, uint8_t excludedPlayerID) {
    for(auto& player : players){
        if(player.connected && player.id != excludedPlayerID){
            player.writeBlockChange(x, y, z, blockID);
        }
    }
}

void minecraft::broadcastPlayerInfo(){
    // calculate data length in a horrible non-automated way for now TODO
    uint32_t num = getPlayerNum();
    // broadcast playerinfo
    for(auto& player : players){
        if(player.connected){
            packet pac(player.S, player.mtx);
            pac.writeVarInt(0x32);
            pac.writeVarInt(0); // action add player
            pac.writeVarInt(num); // number of players
            for(const auto& p : players){
                if(p.connected){
                    pac.writeUUID(p.id); // first player's uuid
                    pac.writeString(p.username);
                    pac.writeVarInt(0); // no properties given
                    pac.writeVarInt(0); // gamemode
                    pac.writeVarInt(100); // hardcoded ping TODO
                    pac.writeBoolean(0); // has display name
                }
            }
            pac.writePacket();
            player.login("Sent player info");
        }
    }
}

uint8_t minecraft::getPlayerNum(){
    uint8_t i = 0;
    for(const auto& player : players){
        if(player.connected) i++;
    }
    return i;
}

String sanitise(String s) {
    return s;
    for (uint16_t index = 0; index < s.length(); index++) {
        if (s[index] == '"') {
            s = s.substring(0, index) + '\\' + s.substring(index);
            index++;
        }
    }
}

// CLIENTBOUND PLAYER
void minecraft::player::writeChat(String msg, String username){
    packet p(S, mtx);
    msg = sanitise(msg);
    String s = "{\"text\": \"<" + username + "> " + msg + "\",\"bold\": \"false\"}";
    p.writeVarInt(0x0E);
    p.writeString(s);
    p.writeByte(0);
    p.writeUUID(id);
    p.writePacket();
}

void minecraft::player::writeLoginSuccess(){
    packet p(S, mtx);
    p.writeVarInt(0x02);
    p.writeUUID(id);
    p.writeString(username);
    p.writePacket();
    logout("login success sent");
}

void minecraft::player::writeFreeHeap() {
    loginfo("Free heap: " + String(esp_get_free_heap_size() / 1000) + " KB");
}

void minecraft::player::writeChunk(uint8_t x, uint8_t y){
    writeFreeHeap();
    packet p(S, mtx);
    p.writeVarInt(0x20); 
    p.writeInt(x); // X
    p.writeInt(y); // Z
    p.writeBoolean(1); // full chunk yes
    p.writeVarInt(0x01); //bitmask set to 0xFF because we're sending the whole chunk
    writeFreeHeap();

    p.write(height_map_NBT, sizeof(height_map_NBT) / sizeof(height_map_NBT[0]));

    uint8_t b[1024];
    memset(b, 127, 1024);
    p.writeVarInt(1024); // array length 2 bytes as varint
    p.write(b, 1024); // 127 = void biome
    writeFreeHeap();
    
    p.writeVarInt(4487); // magic 

    // calculate non-air blocks in chunk
    uint16_t non_air_blocks = 0;
    for(uint8_t i = 0; i < 16; i++){
        for(uint8_t j = 0; j < 16; j++){
            for(uint8_t k = 0; k < 16; k++){
                if(chunk[x][y][i][j][k] != 0){
                    non_air_blocks++;
                }
            }
        }
    }
    writeFreeHeap();

    p.writeShort(non_air_blocks); // non-air blocks 
    p.writeUnsignedByte(8); // bits per block
    p.writeVarInt(256); // palette length 8 bits per block
    p.write(palette, 384); // write palette
    p.writeVarInt(512); // we're sending 512 longs or 4096 bytes
    uint8_t * buf = (uint8_t*)chunk[x][y];
    p.write(buf, 4096);
    writeFreeHeap();

    p.writeVarInt(0); // no block entities
    p.writePacket();
    logout("Chunk sent");
    writeFreeHeap();
}


void minecraft::player::writePlayerPositionAndLook(double x, double y, double z, float _yaw, float _pitch, uint8_t flags){
    packet p(S, mtx);
    p.writeVarInt(0x34);
    p.writeDouble(x);
    p.writeDouble(y);
    p.writeDouble(z);
    p.writeFloat(_yaw);
    p.writeFloat(_pitch);
    p.writeUnsignedByte(flags);
    p.writeVarInt(0x55);
    p.writePacket();
    logout("Player position and look sent");
}

void minecraft::player::writeKeepAlive(){
    packet p(S, mtx);
    p.writeVarInt(0x1F);
    uint32_t num = millis()/1000;
    p.writeLong(num);
    logout("Keepalive sent: " + String(num));
    p.writePacket();
}

void minecraft::player::writeServerDifficulty(){
    packet p(S, mtx);
    p.writeVarInt(0x0D);
    p.writeUnsignedByte(0);
    p.writeBoolean(1);
    logout("Server difficulty packet sent");
    p.writePacket();
}

void minecraft::player::writeSpawnPlayer(double x, double y, double z, int _yaw_i, int _pitch_i, uint8_t id){
    packet p(S, mtx);
    p.writeVarInt(0x04);
    p.writeVarInt(id); // player id
    p.writeUUID(id); // player uuid
    p.writeDouble(x); // player x
    p.writeDouble(y); // player y
    p.writeDouble(z); // player z
    p.writeUnsignedByte(_yaw_i); // player yaw
    p.writeUnsignedByte(_pitch_i); // player pitch
    p.writePacket();
    logout("Spawn player sent id:" + String(id));
}

void minecraft::player::writeJoinGame(){
    String overworld = "minecraft:overworld";
    packet p(S, mtx);
    p.writeVarInt(0x24);
    p.writeInt(id); // entity id
    p.writeBoolean(0); // is hardcore
    p.writeUnsignedByte(0); // gamemode
    p.writeByte(-1); // previous gamemode
    p.writeVarInt(1); // only one world
    p.writeString(overworld); // only one world
    p.write(dimension_codec_NBT, sizeof(dimension_codec_NBT) / sizeof(dimension_codec_NBT[0])); // NBT with world settings
    p.write(dimension_NBT, sizeof(dimension_NBT) / sizeof(dimension_NBT[0])); // NBT with world settings
    p.writeString(overworld); // spawn world
    p.writeLong(0); // hashed seed
    p.writeVarInt(10); // max players
    p.writeVarInt(12); // view distance
    p.writeBoolean(0); // reduced debug info
    p.writeBoolean(1); // enable respawn screen
    p.writeBoolean(0); // is debug world
    p.writeBoolean(1); // is flat
    logout("Join game packet sent");
    p.writePacket();
}

void minecraft::player::writeRespawn() {
    mc->broadcastEntityDestroy(id);
    health = 20;
    fallingHeight = 0;
    food = 20;
    food_sat = 5;
    mc->broadcastEntityHealth(health, id);
    mc->broadcastEntityPose(0, id);
    packet p(S, mtx);
    p.writeVarInt(0x39);
    p.write(dimension_NBT, sizeof(dimension_NBT) / sizeof(dimension_NBT[0])); // NBT with world settings
    p.writeString("minecraft:overworld"); // spawn world
    p.writeLong(0); // hashed seed
    p.writeUnsignedByte(0); // gamemode
    p.writeByte(-1); // previous gamemode
    p.writeBoolean(0); // is debug world
    p.writeBoolean(1); // is flat
    p.writeBoolean(0); // do not copy met
    p.writePacket();
    writeSpawnPackets();
    mc->broadcastSpawnPlayer(id);
    updateEquipment();
}

void minecraft::player::writeInventoryItems() {
    packet p(S, mtx);
    p.writeVarInt(0x13);
    p.writeUnsignedByte(0); // window id
    p.writeShort(46); // slot count
    for (int i = 0; i < 46; i++) {
        p.writeBoolean(inventory[i].present ? 1 : 0); // is present
        if (inventory[i].present) {
            p.writeVarInt(inventory[i].itemID); // id
            p.writeByte(inventory[i].itemCount); // count
            p.writeByte(0); // nbt
        }
    }
    p.writePacket();
}

void minecraft::player::updateEquipment() {
    slot* selectedItem = &inventory[selectedSlot]; // item in hand
    slot* selectedOffhand = &inventory[45]; // item in offhand

    // broadcast held item
    if (selectedItem->present) {
        mc->broadcastEntityEquipment(id, 0, true, selectedItem->itemID, selectedItem->itemCount);
    } else {
        mc->broadcastEntityEquipment(id, 0, false, 0, 0);
    }

    // broadcast held offhand item
    if (selectedOffhand->present) {
        mc->broadcastEntityEquipment(id, 1, true, selectedOffhand->itemID, selectedOffhand->itemCount);
    } else {
        mc->broadcastEntityEquipment(id, 1, false, 0, 0);
    }

    // tell this player about everyone else's equipment
    for(auto& player : mc->players){
        if(player.connected && player.id != id){
            slot* playerSelectedItem = &player.inventory[selectedSlot]; // item in hand
            slot* playerSelectedOffhand = &player.inventory[45]; // item in offhand
            // broadcast held item
            if (playerSelectedItem->present) writeEntityEquipment(player.id, 0, true, playerSelectedItem->itemID, playerSelectedItem->itemCount);

            // broadcast held offhand item
            if (playerSelectedOffhand->present) writeEntityEquipment(player.id, 1, true, playerSelectedOffhand->itemID, playerSelectedOffhand->itemCount);
        }
    }
}

void minecraft::player::writeResponse(){
    String uuid = "00000000-0000-0000-0000-00000000000";
    packet p(S, mtx);
    p.writeVarInt(0);
    p.writeString("{\"version\": {\"name\": \"1.16.5\",\"protocol\": 754},\"players\": {\"max\": 2,\"online\": 2,\"sample\": [{\"name\": \"Woodie's ESP32\",\"id\": \"" + uuid + "0\"},{\"name\": \"MC Server\",\"id\": \"" + uuid + "1\"}]},\"description\": {\"text\": \"esp32 server\"}}");
    logout("response packet sent");
    p.writePacket();
}

void minecraft::player::writePong(uint64_t payload){
    packet p(S, mtx);
    p.writeVarInt(0x01); // packet id
    p.writeLong(payload); // payload
    logout("pong");
    p.writePacket();
}

void minecraft::player::writeEntityTeleport(double x, double y, double z, int _yaw_i, int _pitch_i, bool on_ground, uint8_t id){
    packet p(S, mtx);
    p.writeVarInt(0x56); // packet id
    p.writeVarInt(id);
    p.writeDouble(x);
    p.writeDouble(y);
    p.writeDouble(z);
    p.writeByte(_yaw_i);
    p.writeByte(_pitch_i);
    p.writeBoolean(on_ground);
    p.writePacket();
}

void minecraft::player::writeEntityRotation(int _yaw_i, int _pitch_i, bool on_ground, uint8_t id){
    packet p(S, mtx);
    p.writeVarInt(0x29); // packet id
    p.writeVarInt(id);
    p.writeByte(_yaw_i);
    p.writeByte(_pitch_i);
    p.writeBoolean(on_ground);
    p.writePacket();
}

void minecraft::player::writeEntityLook(int _yaw_i, uint8_t id){
    packet p(S, mtx);
    p.writeVarInt(0x3A); // packet id
    p.writeVarInt(id);
    p.writeByte(_yaw_i);
    p.writePacket();
}

void minecraft::player::writeEntityAnimation(uint8_t anim, uint8_t id){
    packet p(S, mtx);
    p.writeVarInt(0x05); // packet id
    p.writeVarInt(id);
    p.writeByte(anim);
    p.writePacket();
}

void minecraft::player::writeEntityStatus(uint8_t status, uint8_t id){
    packet p(S, mtx);
    p.writeVarInt(0x1A); // packet id
    p.writeInt(id);
    p.writeByte(status);
    p.writePacket();
}

void minecraft::player::writeEntityPose(uint8_t pose, uint8_t id){
    packet p(S, mtx);
    p.writeVarInt(0x44); // packet id
    p.writeVarInt(id);
    p.writeUnsignedByte(6); // field unique id
    p.writeVarInt(18); // we need only poses since swimming etc. isn't supported
    p.writeVarInt(pose);
    p.writeUnsignedByte(0xFF); // terminate entity metadata array
    p.writePacket();
}

void minecraft::player::writeEntityHealth(float health, uint8_t entityID) {
    packet p(S, mtx);
    p.writeVarInt(0x44); 
    p.writeVarInt(entityID);
    p.writeUnsignedByte(8); 
    p.writeVarInt(2);
    p.writeFloat(health);
    p.writeUnsignedByte(0xFF);
    p.writePacket();
}

void minecraft::player::writeEntityDestroy(uint8_t id){
    packet p(S, mtx);
    p.writeVarInt(0x36); // packet id
    p.writeVarInt(1); // entity count
    p.writeVarInt(id);
    p.writePacket();
}

void minecraft::player::writeEntityEquipment(uint8_t entityID, uint8_t slot, bool present, uint16_t itemID, uint8_t itemCount) {
    packet p(S, mtx);
    p.writeVarInt(0x47); // packet id
    p.writeVarInt(entityID); 
    p.writeByte(slot); 
    p.writeBoolean(present); 
    if (present) {
        p.writeVarInt(itemID);
        p.writeByte(itemCount);
        p.writeByte(0x00); // no nbt
    }
    p.writePacket();
}

void minecraft::player::writeInventorySlot(bool present, uint16_t slot, uint32_t itemID, uint8_t itemCount) {
    packet p(S, mtx);
    p.writeVarInt(0x15); // packet id
    p.writeByte(0); // window id (0 for inventory)
    p.writeShort(slot); // slot index
    p.writeBoolean(present ? 1 : 0); // is present
    if(present){
        p.writeVarInt(itemID); // item id
        p.writeByte(itemCount); // item count
        p.writeByte(0); // no nbt
    }
    p.writePacket();
}


void minecraft::player::writeBlockChange(int64_t x, int64_t y, int64_t z, uint16_t blockID) {
    packet p(S, mtx);
    p.writeVarInt(0x0B);
    p.writePosition(x, y, z);
    p.writeVarInt(blockID);
    p.writePacket();
}

void minecraft::player::writeBlockBreakAnimation(uint8_t id, int64_t x, int64_t y, int64_t z, uint8_t stage) {
    packet p(S, mtx);
    p.writeVarInt(0x08);
    p.writeVarInt(id);
    p.writePosition(x, y, z);
    p.writeByte(stage);
    p.writePacket();
}

void minecraft::player::writeHealth() {
    packet p(S, mtx);
    p.writeVarInt(0x49); // packet id
    p.writeFloat(health); // packet id
    p.writeVarInt(food);
    p.writeFloat(food_sat);
    p.writePacket();
}

// READ TYPES
uint16_t minecraft::player::readUnsignedShort(){
    return readShort();
}

int16_t minecraft::player::readShort(){
    while(S->available() < 2);
    int ret = S->read();
    return (ret << 8) | S->read();
}

float minecraft::player::readFloat(){
    char b[4] = {};
    while(S->available() < 4);
    for(int i=3; i>=0; i--){
        b[i] = S->read();
    }
    float f = 0;
    memcpy(&f, b, sizeof(float));
    return f;
}

double minecraft::player::readDouble(){
    char b[8] = {};
    while(S->available() < 8);
    for(int i=7; i>=0; i--){
        b[i] = S->read();
    }
    double d = 0;
    memcpy(&d, b, sizeof(double));
    return d;
}

int64_t minecraft::player::readLong(){
    return readUnsignedLong();
}

uint64_t minecraft::player::readUnsignedLong() {
    char b[8] = {};
    while(S->available() < 8);
    for(int i=0; i<8; i++){
        b[i] = S->read();
    }
    uint64_t l = ((uint64_t) b[0] << 56)
       | ((uint64_t) b[1] & 0xff) << 48
       | ((uint64_t) b[2] & 0xff) << 40
       | ((uint64_t) b[3] & 0xff) << 32
       | ((uint64_t) b[4] & 0xff) << 24
       | ((uint64_t) b[5] & 0xff) << 16
       | ((uint64_t) b[6] & 0xff) << 8
       | ((uint64_t) b[7] & 0xff);
    return l;
}

String minecraft::player::readString(){
    int length = readVarInt();
    String result;
    for(int i=0; i<length; i++){
        while (S->available() < 1);
        result.concat((char)S->read());
    }
    return result;
}

int32_t minecraft::player::readVarInt() {
    int numRead = 0;
    int result = 0;
    byte read;
    do {
        while (S->available() < 1);
        read = S->read();
        int value = (read & 0b01111111);
        result |= (value << (7 * numRead));
        numRead++;
        if (numRead > 5) {
            logerr("VarInt too big");
        }
    } while ((read & 0b10000000) != 0);
    return result;
}

uint8_t minecraft::player::readByte(){
    return S->read();
}

bool minecraft::player::readBool(){
    return S->read();
}

// WRITE TYPES
void packet::writeDouble(double value){
    unsigned char * p = reinterpret_cast<unsigned char *>(&value);
    for(int i=7; i>=0; i--){
        write(p[i]);
    }
}

void packet::writeFloat(float value) {
    unsigned char * p = reinterpret_cast<unsigned char *>(&value);
    for(int i=3; i>=0; i--){
        write(p[i]);
    }
}

void packet::writeVarInt(int32_t value) {
    do {
        uint8_t temp = (uint8_t)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        write(temp);
    } while (value != 0);
}

void packet::writeVarLong(int64_t value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        write(temp);
    } while (value != 0);
}

void packet::writeString(String str){
    int length = str.length();
    byte buf[length + 1]; 
    str.getBytes(buf, length + 1);
    writeVarInt(length);
    write(buf, length);
    /*for(int i=0; i<length; i++){
        write(buf[i]);
    }*/
}

void packet::writeLong(int64_t num){
    for(int i=7; i>=0; i--){
        write((uint8_t)((num >> (i*8)) & 0xff));
    }
}

void packet::writeUnsignedLong(uint64_t num){
    for(int i=7; i>=0; i--){
        write((uint8_t)((num >> (i*8)) & 0xff));
    }
}

void packet::writeUnsignedShort(uint16_t num){
    write((byte)((num >> 8) & 0xff));
    write((byte)(num & 0xff));
}

void packet::writeUnsignedByte(uint8_t num){
    write(num);
}

void packet::writeInt(int32_t num){
    write((byte)((num >> 24) & 0xff));
    write((byte)((num >> 16) & 0xff));
    write((byte)((num >> 8) & 0xff));
    write((byte)(num & 0xff));
}

void packet::writeShort(int16_t num){
    write((byte)((num >> 8) & 0xff));
    write((byte)(num & 0xff));
}

void packet::writeByte(int8_t num){
    write(num);
}

void packet::writeBoolean(uint8_t val){
    write(val);
}

void packet::writeUUID(int user_id){
    uint8_t b[15] = {0};
    write(b, 15);
    write(user_id);
}

void packet::writePosition(int64_t x, int64_t y, int64_t z) {
    uint64_t pos = ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF);
    for(int i=7; i>=0; i--){
        write((uint8_t)((pos >> (i*8)) & 0xff));
    }
}

void minecraft::player::writeLength(uint32_t length){
    do {
        uint8_t temp = (uint8_t)(length & 0b01111111);
        length = lsr(length,7);
        if (length != 0) {
            temp |= 0b10000000;
        }
        S->write(temp);
    } while (length != 0);
}

void minecraft::player::writeSpawnPackets() {
    double spawnX = 8.5;
    double spawnZ = 8.5;

    uint8_t spawnY;

    uint8_t spawnBlockX = std::floor(spawnX);
    uint8_t spawnBlockZ = std::floor(spawnZ);

    int8_t chunkX = spawnBlockX / 16;
    int8_t chunkZ = spawnBlockZ / 16;

    uint8_t chunkBlockArrayX;
    if (spawnBlockX < 8) {
        chunkBlockArrayX = 7 - spawnBlockX;
    } else {
        chunkBlockArrayX = 15 - (spawnBlockX  - 8);
    }
    for (uint8_t i = 0; i < 16; i++) {
        if (chunk[chunkX][chunkZ][i][spawnBlockZ][chunkBlockArrayX] == 0x00 && chunk[chunkX][chunkZ][i + 1][spawnBlockZ][chunkBlockArrayX] == 0x00) {
            spawnY = i;
            break;
        }
    }
    if (spawnY == 0) spawnY = 16;

    x = spawnX;
    y = spawnY;
    z = spawnZ;

    writePlayerPositionAndLook(spawnX, spawnY, spawnZ, 0, 0, 0x00);
    writeServerDifficulty();
    for (uint8_t i = 0; i < 2; i++) {
        for (uint8_t j = 0; j < 2; j++) {
            writeChunk(i, j);
        }
    }
    writeInventoryItems();
}

// HANDLERS
bool minecraft::player::join(){
    uint8_t res = readHandShake();
    if(res == 1){
        readRequest();
        delay(1000);
        writeResponse();
        uint64_t payload = readPing();
        writePong(payload);
        return false;
    } else if(res == 2){
        if(!readLoginStart()) return false;
        writeLoginSuccess();
    }
    connected = true;
    writeJoinGame();
    writeSpawnPackets();
    mc->broadcastPlayerInfo();
    mc->broadcastChatMessage(username + " joined the server", "Server");
    mc->broadcastSpawnPlayer(id);

    updateEquipment();
    return true;
}

void minecraft::handle(){
    for(auto& player : players){
        if(player.connected){
            player.writeKeepAlive();
        }
    }
}

void minecraft::player::handle(){
    if(S->available() > 2){
        uint32_t length = S->read();
        uint32_t packetid = S->read();
        switch (packetid){
        case 0x03:
            readChat();
            break;
        case 0x04:
            readClientStatus();
            break;
        case 0x09:
            readClickWindow();
            break;
        case 0x0E:
            readInteractEntity();
            break;
        case 0x12:
            readPosition();
            break;
        case 0x14:
            readRotation();
            break;
        case 0x10:
            readKeepAlive();
            break;
        case 0x13:
            readPositionAndLook();
            break;
        case 0x00:
            readTeleportConfirm();
            break;
        case 0x25:
            readHeldItem();
            break;
        case 0x2C:
            readAnimation();
            break;
        case 0x1B:
            readAction();
            break;
        case 0x1C:
            readEntityAction();
            break;
        case 0x2E:
            readBlockPlacement();
            break;
        default:
            loginfo("id: 0x" + String(packetid, HEX) + " length: " + String(length));
            for(int i=0; i < length - VarIntLength(packetid); i++ ){
                while (S->available() < 1);
                // loginfo("packet id " + String(packetid));
                S->read();
            }
            break;
        }
    }
}

void minecraft::player::death(String message) {
    mc->broadcastChatMessage(message, "Server");
    mc->broadcastEntityPose(6, id); // play death animation
    mc->broadcastEntityStatus(3, id); // play death sound and animation
}

// UTILITIES
void minecraft::player::loginfo(String msg){
    Serial.println( "[INFO] p" + String(id) + " " + msg);
}

void minecraft::player::logerr(String msg){
    Serial.println( "[ERROR] p" + String(id) + " " + msg);
}

void minecraft::player::login(String msg){
    Serial.println( "[INFO] p" + String(id) + " <- " + msg);
}

void minecraft::player::logout(String msg){
    Serial.println( "[INFO] p" + String(id) + " -> " + msg);
}

uint8_t minecraft::player::findFreeInvSlot(uint16_t itemID){
    uint8_t bestCandidate = 0;
    uint8_t bestCandidateType = 0;
    String s = " slot ";
    for (uint8_t i = 36; i < 46; i++){
        loginfo("Checking" + s + String(i));
        uint8_t slotFree = isSlotFree(i, itemID);
        if (slotFree == 2 && bestCandidateType != 2){
            loginfo("Found perfect" + s + String(i));
            return i;
        } else if (slotFree == 1 && bestCandidateType == 0){
            loginfo("Found free" + s + String(i));
            bestCandidate = i;
            bestCandidateType = 1;
        }
    }
    if (bestCandidate == 1) return bestCandidate;
    for (uint8_t i = 9; i < 36; i++){
        uint8_t slotFree = isSlotFree(i, itemID);
        if (slotFree == 2 && bestCandidateType != 2){
            return i;
        } else if (slotFree == 1 && bestCandidateType == 0){
            bestCandidate = i;
            bestCandidateType = 1;
        }
    }
    return bestCandidate;
}

uint8_t minecraft::player::isSlotFree(uint8_t slot, uint16_t itemID){
    if (inventory[slot].present) {
        if (inventory[slot].itemID == itemID && inventory[slot].itemCount < 64) {
            return 2;
        }
        return 0;
    } else {
        return 1;
    }
}

uint16_t minecraft::player::blockToItem(uint16_t blockID) {
    auto it = blockStateToItemMap.find(blockID);  // find the key
    if (it != blockStateToItemMap.end()) {
        return it->second;
    } else {
        return 0;
    }
}

uint16_t minecraft::player::itemToBlock(uint16_t itemID) {
    for (auto const& pair : blockStateToItemMap) {
        if (pair.second == itemID) {
            return pair.first;
        }
    }
    return 0;
}

int32_t lsr(int32_t x, uint32_t n){
  return (int32_t)((uint32_t)x >> n);
}

uint32_t minecraft::player::VarIntLength(int val) {
    if(val == 0){
        return 1;
    }
    return (int)floor(log(val) / log(128)) + 1;
}

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}
