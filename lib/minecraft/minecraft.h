#ifndef MINECRAFT_H
#define MINECRAFT_H

#include <Arduino.h>
#include <mutex>
#include <array>
#include <map>

typedef std::map<uint16_t, int16_t> BlockStateToItemMap;

const BlockStateToItemMap blockStateToItemMap = {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7}, {8, 8}, {10, 9}, {11, 10}, {12, 11}, {14, 14}, {15, 15}, {16, 16}, {17, 17}, {18, 18}, {19, 19}, {20, 20}, {21, 23}, {23, 24}, {25, 25}, {27, 26}, {29, 27}, {31, 28}, {33, 29}, {66, 30}, {67, 31}, {68, 32}, {69, 33}, {70, 34}, {71, 35}, {72, 36}, {73, 37}, {76, 38}, {79, 39}, {82, 40}, {85, 41}, {88, 42}, {91, 46}, {94, 47}, {97, 48}, {100, 49}, {103, 50}, {106, 45}, {109, 61}, {112, 62}, {115, 63}, {118, 64}, {121, 65}, {124, 66}, {127, 53}, {130, 54}, {133, 55}, {136, 56}, {139, 57}, {142, 58}, {145, 69}, {159, 70}, {173, 71}, {187, 72}, {201, 73}, {215, 74}, {229, 75}, {230, 76}, {231, 77}, {232, 78}, {233, 79}, {234, 80}, {246, 81}, {247, 82}, {248, 83}, {249, 84}, {1049, 716}, {1065, 717}, {1081, 718}, {1097, 719}, {1113, 720}, {1129, 721}, {1145, 722}, {1161, 723}, {1177, 724}, {1193, 725}, {1209, 726}, {1225, 727}, {1241, 728}, {1257, 729}, {1273, 730}, {1289, 731}, {1305, 85}, {1317, 86}, {1329, 87}, {1341, 88}, {1342, 89}, {1343, 90}, {1344, 91}, {1345, 92}, {1348, 94}, {1384, 95}, {1385, 96}, {1386, 97}, {1387, 98}, {1388, 99}, {1389, 100}, {1390, 101}, {1391, 102}, {1392, 103}, {1393, 104}, {1394, 105}, {1395, 106}, {1396, 107}, {1397, 108}, {1398, 109}, {1399, 110}, {1412, 111}, {1413, 112}, {1414, 113}, {1415, 114}, {1416, 115}, {1417, 116}, {1418, 117}, {1419, 118}, {1420, 119}, {1421, 120}, {1422, 121}, {1423, 123}, {1424, 122}, {1425, 124}, {1426, 125}, {1427, 136}, {1428, 137}, {1429, 166}, {1430, 167}, {1432, 168}, {1433, 169}, {1434, 170}, {1435, 171}, {1953, 178}, {1954, 179}, {2034, 180}, {3354, 181}, {3355, 182}, {3356, 183}, {3357, 620}, {3365, 184}, {3373, 185}, {3381, 652}, {3413, 653}, {3445, 654}, {3477, 656}, {3509, 655}, {3541, 657}, {3573, 558}, {3637, 186}, {3645, 187}, {3655, 188}, {3783, 189}, {3807, 190}, {3809, 557}, {3873, 191}, {3875, 192}, {3877, 193}, {3879, 194}, {3881, 195}, {3883, 196}, {3885, 200}, {3887, 201}, {3897, 304}, {3921, 202}, {3929, 203}, {3930, 204}, {3931, 205}, {3947, 206}, {3948, 133}, {3964, 207}, {3966, 208}, {3998, 216}, {3999, 218}, {4000, 219}, {4001, 220}, {4002, 221}, {4005, 222}, {4008, 223}, {4013, 224}, {4016, 217}, {4020, 225}, {4024, 715}, {4031, 566}, {4095, 379}, {4096, 380}, {4097, 381}, {4098, 382}, {4099, 383}, {4100, 384}, {4101, 385}, {4102, 386}, {4103, 387}, {4104, 388}, {4105, 389}, {4106, 390}, {4107, 391}, {4108, 392}, {4109, 393}, {4110, 394}, {4111, 226}, {4175, 227}, {4239, 228}, {4303, 229}, {4367, 230}, {4431, 231}, {4495, 240}, {4496, 241}, {4497, 242}, {4498, 243}, {4499, 234}, {4500, 235}, {4501, 236}, {4502, 237}, {4503, 238}, {4504, 239}, {4505, 244}, {4569, 245}, {4633, 246}, {4697, 247}, {4729, 248}, {4735, 249}, {4767, 250}, {4792, 251}, {4824, 252}, {4856, 260}, {4936, 261}, {5016, 262}, {5018, 263}, {5019, 264}, {5020, 267}, {5052, 268}, {5132, 748}, {5136, 269}, {5137, 755}, {5145, 756}, {5150, 270}, {5158, 271}, {5159, 273}, {5160, 274}, {5174, 275}, {5254, 276}, {5255, 277}, {5263, 278}, {5407, 279}, {5408, 280}, {5488, 281}, {5568, 282}, {5648, 285}, {5660, 286}, {5661, 287}, {5985, 288}, {6309, 829}, {6350, 305}, {6374, 306}, {6398, 307}, {6422, 308}, {6446, 309}, {6470, 310}, {6494, 836}, {6514, 837}, {6534, 839}, {6554, 838}, {6574, 840}, {6594, 841}, {6614, 314}, {6618, 315}, {6622, 316}, {6626, 317}, {6650, 318}, {6666, 319}, {6682, 567}, {6698, 320}, {6730, 321}, {6731, 322}, {6732, 323}, {6742, 325}, {6743, 324}, {6744, 327}, {6747, 328}, {6827, 329}, {6839, 330}, {6851, 331}, {6852, 332}, {6853, 333}, {6854, 334}, {6855, 335}, {6856, 336}, {6857, 337}, {6858, 338}, {6859, 339}, {6860, 340}, {6861, 341}, {6862, 342}, {6863, 343}, {6864, 344}, {6865, 345}, {6866, 346}, {6867, 395}, {6899, 396}, {6931, 397}, {6963, 398}, {6995, 399}, {7027, 400}, {7059, 401}, {7091, 402}, {7123, 403}, {7155, 404}, {7187, 405}, {7219, 406}, {7251, 407}, {7283, 408}, {7315, 409}, {7347, 410}, {7379, 369}, {7459, 370}, {7539, 371}, {7540, 347}, {7541, 348}, {7605, 411}, {7606, 412}, {7607, 413}, {7608, 414}, {7688, 415}, {7768, 416}, {7848, 159}, {7854, 160}, {7860, 161}, {7866, 417}, {7867, 349}, {7870, 350}, {7871, 351}, {7872, 352}, {7873, 353}, {7874, 354}, {7875, 355}, {7876, 356}, {7877, 357}, {7878, 358}, {7879, 359}, {7880, 360}, {7881, 361}, {7882, 362}, {7883, 363}, {7884, 364}, {7885, 365}, {7886, 366}, {7887, 367}, {7888, 368}, {7889, 373}, {7891, 374}, {7893, 375}, {7895, 376}, {7897, 377}, {7899, 378}, {7901, 870}, {7917, 871}, {7933, 872}, {7949, 873}, {7965, 874}, {7981, 875}, {7997, 876}, {8013, 877}, {8029, 878}, {8045, 879}, {8061, 880}, {8077, 881}, {8093, 882}, {8109, 883}, {8125, 884}, {8141, 885}, {8221, 418}, {8222, 419}, {8223, 420}, {8224, 421}, {8304, 138}, {8310, 139}, {8316, 140}, {8322, 141}, {8328, 142}, {8334, 143}, {8340, 146}, {8346, 147}, {8352, 148}, {8358, 149}, {8364, 150}, {8370, 151}, {8376, 152}, {8382, 153}, {8388, 154}, {8394, 155}, {8400, 156}, {8406, 157}, {8412, 158}, {8418, 165}, {8419, 164}, {8420, 162}, {8421, 163}, {8422, 253}, {8454, 254}, {8486, 255}, {8518, 256}, {8550, 257}, {8582, 209}, {8614, 210}, {8646, 211}, {8678, 212}, {8710, 213}, {8742, 559}, {8806, 560}, {8870, 561}, {8934, 562}, {8998, 563}, {9062, 172}, {9068, 173}, {9132, 174}, {9138, 175}, {9139, 176}, {9142, 177}, {9222, 272}, {9227, 372}, {9229, 422}, {9241, 423}, {9257, 424}, {9258, 425}, {9259, 427}, {9260, 428}, {9263, 429}, {9264, 430}, {9276, 431}, {9282, 432}, {9288, 433}, {9294, 434}, {9300, 435}, {9306, 436}, {9312, 437}, {9318, 438}, {9324, 439}, {9330, 440}, {9336, 441}, {9342, 442}, {9348, 443}, {9354, 444}, {9360, 445}, {9366, 446}, {9372, 447}, {9378, 448}, {9382, 449}, {9386, 450}, {9390, 451}, {9394, 452}, {9398, 453}, {9402, 454}, {9406, 455}, {9410, 456}, {9414, 457}, {9418, 458}, {9422, 459}, {9426, 460}, {9430, 461}, {9434, 462}, {9438, 463}, {9442, 464}, {9443, 465}, {9444, 466}, {9445, 467}, {9446, 468}, {9447, 469}, {9448, 470}, {9449, 471}, {9450, 472}, {9451, 473}, {9452, 474}, {9453, 475}, {9454, 476}, {9455, 477}, {9456, 478}, {9457, 479}, {9458, 480}, {9459, 481}, {9460, 482}, {9461, 483}, {9462, 484}, {9463, 485}, {9464, 486}, {9465, 487}, {9466, 488}, {9467, 489}, {9468, 490}, {9469, 491}, {9470, 492}, {9471, 493}, {9472, 494}, {9473, 495}, {9474, 134}, {9501, 676}, {9502, 496}, {9514, 497}, {9515, 498}, {9516, 499}, {9517, 500}, {9518, 501}, {9519, 502}, {9520, 503}, {9521, 504}, {9522, 505}, {9523, 506}, {9524, 516}, {9526, 512}, {9528, 513}, {9530, 514}, {9532, 515}, {9534, 507}, {9536, 508}, {9538, 509}, {9540, 510}, {9542, 511}, {9544, 522}, {9546, 523}, {9548, 524}, {9550, 525}, {9552, 526}, {9554, 517}, {9556, 518}, {9558, 519}, {9560, 520}, {9562, 521}, {9644, 93}, {9652, 527}, {9653, 528}, {9656, 135}, {9673, 529}, {9753, 530}, {9833, 531}, {9913, 532}, {9993, 533}, {10073, 534}, {10153, 535}, {10233, 536}, {10313, 537}, {10393, 538}, {10473, 539}, {10553, 540}, {10633, 541}, {10713, 542}, {10793, 543}, {10799, 544}, {10805, 545}, {10811, 546}, {10817, 547}, {10823, 548}, {10829, 549}, {10835, 550}, {10841, 551}, {10847, 552}, {10853, 553}, {10859, 554}, {10865, 555}, {10871, 289}, {11195, 290}, {11519, 291}, {11843, 292}, {12167, 293}, {12491, 294}, {12815, 295}, {13139, 296}, {13463, 297}, {13787, 298}, {14111, 299}, {14435, 300}, {14759, 556}, {14791, 928}, {14795, 936}, {14807, 937}, {14815, 938}, {14823, 939}, {14824, 940}, {14825, 941}, {14837, 942}, {14853, 943}, {14854, 944}, {14858, 945}, {14890, 946}, {14894, 947}, {14898, 949}, {14930, 950}, {14966, 44}, {14969, 52}, {14972, 68}, {14975, 60}, {14978, 13}, {14979, 127}, {14980, 426}, {14981, 129}, {14982, 130}, {14983, 43}, {14986, 51}, {14989, 67}, {14992, 59}, {14995, 12}, {14996, 126}, {14997, 951}, {14998, 131}, {15025, 132}, {15052, 128}, {15053, 21}, {15054, 22}, {15055, 144}, {15061, 145}, {15067, 197}, {15069, 198}, {15071, 214}, {15103, 215}, {15135, 232}, {15199, 233}, {15263, 258}, {15295, 259}, {15327, 283}, {15407, 284}, {15487, 311}, {15511, 312}, {15535, 564}, {15599, 565}, {15663, 658}, {15695, 659}, {15743, 568}, {15747, 569}, {15759, 935}, {15768, 961}, {15784, 953}, {15808, 954}, {15832, 956}, {15833, 957}, {15834, 959}, {15835, 960}, {15836, 962}, {15837, 975}, {15846, 958}, {15847, 963}, {15848, 965}, {15928, 301}, {16252, 964}, {16258, 967}, {16259, 971}, {16260, 974}, {16261, 970}, {16262, 972}, {16268, 973}, {16348, 303}, {16672, 966}, {16673, 969}, {16753, 968}, {16759, 199}, {16761, 313}, {16785, 302}, {17109, 266}, {17110, 265}, {17111, 326}};

class packet{
    public:
    uint8_t buffer[6000];
    uint32_t index = 0;
    Stream* S;
    std::mutex * mtx;

    packet(Stream* __S, std::mutex * _mtx){
        S = __S;
        mtx = _mtx;
    }

    void write(const uint8_t val);
    void write(const uint8_t * buf, size_t size);
    void writePacket();

    void writeDouble        (double value);
    void writeFloat         (float value);
    void writeVarInt        (int32_t value);
    void writeVarLong       (int64_t value);
    void writeString        (String str);
    void writeUnsignedLong  (uint64_t num);
    void writeUnsignedShort (uint16_t num);
    void writeUnsignedByte  (uint8_t num);
    void writeLong          (int64_t num);
    void writeInt           (int32_t num);
    void writeShort         (int16_t num);
    void writeByte          (int8_t num);
    void writeBoolean       (uint8_t val);
    void writeUUID          (int user_id);
    void writePosition      (int64_t x, int64_t y, int64_t z);

    // write to client
    void serverWriteVarInt  (int32_t value);
};

typedef struct {
    bool present;
    uint16_t itemID;
    uint8_t itemCount;
} slot;

class minecraft{
    public:
    class player{
        public:
        std::mutex* mtx;
        Stream* S;
        minecraft* mc;
        bool connected = false;
        String username;
        double x = 8;
        double y = 5;
        double z = 8;
        double yaw = 0;
        double pitch = 0;
        int yaw_i = 0;
        int pitch_i = 0;
        bool on_ground = true;
        double fallingHeight = 0;
        float health = 20;
        uint8_t food = 20;
        float food_sat = 5;
        uint8_t id = 0;
        uint8_t selectedSlot = 36;
        slot currentSelectedItem = {false}; // used when dragging thing around in the inventory
        slot inventory[46] = {{false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {false}, {true, 0x45, 64}, {true, 0x01, 32}, {true, 0x01, 64}, {true, 0x02, 64}};
        

        player(){  // since mutex is neither moveable or copyable we make a new instance in the constructor
            mtx = new std::mutex();
        }

        bool join               ();
        void handle             ();

        void death              (String message);

        uint8_t readHandShake   ();
        bool readLoginStart     ();
        uint64_t readPing       ();
        void readRequest        ();

        void readChat           ();
        void handleOnGround     (bool onGround, double y);
        void readPosition       ();
        void readRotation       ();
        void readKeepAlive      ();
        void readPositionAndLook();
        void readTeleportConfirm();
        void readHeldItem       ();
        void readAnimation      ();
        void readEntityAction   ();
        void readAction         ();
        void readClientStatus   ();
        void readClickWindow    ();
        void readInteractEntity ();
        void readBlockPlacement ();
        void readUseItem        ();

        void writeResponse      ();
        void writeLoginSuccess  ();
        void writeFreeHeap      ();
        void writeChunk         (uint8_t x, uint8_t y);
        void writePlayerPositionAndLook(double x, double y, double z, float yaw, float pitch, uint8_t flags);
        void writeKeepAlive     ();
        void writeServerDifficulty();
        void writeSpawnPlayer   (double x, double y, double z, int yaw, int pitch, uint8_t id);
        void writeJoinGame      ();
        void writePong          (uint64_t payload);
        void writeChat          (String msg, String username);
        void writeEntityTeleport(double x, double y, double z, int yaw, int pitch, bool on_ground, uint8_t id);
        void writeEntityRotation(int yaw, int pitch, bool on_ground, uint8_t id);
        void writeEntityLook    (int yaw, uint8_t id);
        void writeEntityAnimation(uint8_t anim, uint8_t id);
        void writeEntityStatus  (uint8_t status, uint8_t id);
        void writeEntityPose    (uint8_t pose, uint8_t id);
        void writeEntityHealth  (float health, uint8_t entityID);
        void writeEntityDestroy (uint8_t id);
        void writeEntityEquipment(uint8_t id, uint8_t slot, bool present, uint16_t itemID, uint8_t itemCount);
        void writeInventorySlot (bool present, uint16_t slot, uint32_t itemID, uint8_t itemCount);
        void writeBlockChange   (int64_t x, int64_t y, int64_t z, uint16_t blockID);
        void writeBlockBreakAnimation(uint8_t id, int64_t x, int64_t y, int64_t z, uint8_t stage);
        void writeHealth        ();
        void writeSpawnPackets  ();
        void writeRespawn       ();
        void writeInventoryItems();

        void updateEquipment    ();

        void loginfo            (String msg);
        void logerr             (String msg);
        void login              (String msg);
        void logout             (String msg);

        uint8_t findFreeInvSlot (uint16_t itemID);
        uint8_t isSlotFree      (uint8_t slot, uint16_t itemID);

        uint16_t blockToItem    (uint16_t blockID);
        uint16_t itemToBlock    (uint16_t itemID);

        float readFloat         ();
        double readDouble       ();
        int32_t readVarInt      ();
        String readString       ();
        int64_t readLong        ();
        uint16_t readUnsignedShort();
        int16_t readShort       ();
        uint64_t readUnsignedLong();
        uint32_t VarIntLength   (int val);
        uint8_t readByte        ();
        bool readBool           ();

        void writeLength        (uint32_t length);
    };

    uint64_t tick = 0;
    uint64_t prev_keepalive = 0;
    player players[5];
    bool ledState = false;

    void handle                      ();
    void broadcastChunk              (int8_t chunkX, int8_t chunkZ, uint8_t excludedPlayerID);
    void broadcastChatMessage        (String msg, String username);
    void broadcastSpawnPlayer        (uint8_t id);
    void broadcastPlayerPosAndLook   (double x, double y, double z, int yaw, int pitch, bool on_ground, uint8_t id);
    void broadcastPlayerInfo         ();
    void broadcastPlayerRotation     (int yaw, int pitch, bool on_ground, uint8_t id);
    void broadcastEntityAnimation    (uint8_t anim, uint8_t id, bool sendToSelf = false);
    void broadcastEntityPose         (uint8_t pose, uint8_t id);
    void broadcastEntityHealth       (float health, uint8_t id);
    void broadcastEntityStatus       (uint8_t status, uint8_t id, bool sendToSelf = false);
    void broadcastEntityDestroy      (uint8_t id);
    void broadcastEntityEquipment    (uint8_t id, uint8_t slot, bool present, uint16_t itemID, uint8_t itemCount);
    void broadcastBlockChange        (int64_t x, int64_t y, int64_t z, uint16_t blockID, uint8_t excludedPlayerID);
    void toggleLED                   ();
    uint8_t getPlayerNum             ();
};

int32_t lsr(int32_t x, uint32_t n);
float fmap(float x, float in_min, float in_max, float out_min, float out_max);

#endif