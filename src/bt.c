#include "headers.h"

struct psvs_ds3_input_report_t {
    unsigned char report_id;
    unsigned char unk0;

    unsigned char buttons0;
    unsigned char buttons1;

    unsigned char ps       : 1;
    unsigned char not_used : 7;

    unsigned char unk1;

    unsigned char left_x;
    unsigned char left_y;
    unsigned char right_x;
    unsigned char right_y;

    unsigned int unk2;

    unsigned char up_sens;
    unsigned char right_sens;
    unsigned char down_sens;
    unsigned char left_sens;

    unsigned char L2_sens;
    unsigned char R2_sens;
    unsigned char L1_sens;
    unsigned char R1_sens;

    unsigned char triangle_sens;
    unsigned char circle_sens;
    unsigned char cross_sens;
    unsigned char square_sens;

    unsigned short unk3;
    unsigned char unk4;

    unsigned char status;
    unsigned char power_rating;
    unsigned char comm_status;
    unsigned int unk5;
    unsigned int unk6;
    unsigned char unk7;

    unsigned short accel_x;
    unsigned short accel_y;
    unsigned short accel_z;

    unsigned short gyro_z;
} __attribute__((packed, aligned(32)));

struct psvs_ds4_input_report_t {
    unsigned char report_id;
    unsigned char left_x;
    unsigned char left_y;
    unsigned char right_x;
    unsigned char right_y;

    unsigned char buttons0;
    unsigned char buttons1;

    unsigned char ps   : 1;
    unsigned char tpad : 1;
    unsigned char cnt1 : 6;

    unsigned char l_trigger;
    unsigned char r_trigger;

    unsigned char cnt2;
    unsigned char cnt3;

    unsigned char battery;

    // This is the angular velocity for each axis
    signed short gyro_x;     // +X is forward to up (around left-right axis)
    signed short gyro_y;     // +Y is right to forward (around up-down axis)
    signed short gyro_z;     // +Z is right to up (around forward-backward axis)

    // This is the direction of acceleration (gravity), compared to the controller
    signed short accel_x;    // +X is left
    signed short accel_y;    // +Y is down
    signed short accel_z;    // +Z is forward

    unsigned char unk1[5];

    unsigned char battery_level : 4;
    unsigned char usb_plugged   : 1;
    unsigned char headphones    : 1;
    unsigned char microphone    : 1;
    unsigned char padding       : 1;

    unsigned char unk2[2];
    unsigned char trackpadpackets;
    unsigned char packetcnt;

    unsigned int finger1_id        : 7;
    unsigned int finger1_activelow : 1;
    unsigned int finger1_x         : 12;
    unsigned int finger1_y         : 12;

    unsigned int finger2_id        : 7;
    unsigned int finger2_activelow : 1;
    unsigned int finger2_x         : 12;
    unsigned int finger2_y         : 12;
} __attribute__((packed, aligned(32)));

typedef struct psvs_ds3_input_report_t psvs_ds3_input_report_t;
typedef struct psvs_ds4_input_report_t psvs_ds4_input_report_t;

#define PSVS_BT_CONNECTION_TIMEOUT (5 * 1000 * 1000)
#define PSVS_BT_PACKET_TIMEOUT (1 * 1000 * 1000)

// Bluetooth events seen through ksceBtReadEvent (same ids as ds34vita / ds34motion use)
#define PSVS_BT_EVENT_CONNECTED    0x05 // Connection accepted
#define PSVS_BT_EVENT_DISCONNECTED 0x06 // Device disconnected
#define PSVS_BT_EVENT_HID_READ     0x0A // Reply to a type-0 (read) HID request: its buffer now holds a report

// HID request types (SceBtHidRequest.type)
#define PSVS_BT_HID_REQUEST_READ 0

// Input report ids
#define PSVS_DS3_REPORT_ID 0x01
#define PSVS_DS4_REPORT_ID 0x11

// Error codes returned by SceBt for a DualShock 3, which does not report VID/PID or a name
// (heuristic borrowed from ds34vita / ds34motion)
#define PSVS_BT_ERROR_NO_VID_PID 0x802F5001
#define PSVS_BT_ERROR_NO_NAME    0x802F0C01

// Upper bound when walking a HID request chain (ds34vita links a request to itself)
#define PSVS_BT_MAX_REQUEST_CHAIN 8

typedef enum psvs_gamepad_type_t {
    PSVS_GAMEPAD_NONE = 0,
    PSVS_GAMEPAD_DS3,
    PSVS_GAMEPAD_DS4,
} psvs_gamepad_type_t;

#define PSVS_TOUCH_MAX_FRAMES 4

typedef struct psvs_touch_point_t {
    uint8_t id;
    uint8_t r0;
    uint16_t x;
    uint16_t y;
} psvs_touch_point_t;

typedef struct psvs_touch_frame_t {
    uint64_t timestamp;
    int8_t port;
    int8_t count;
    psvs_touch_point_t points[2];
} psvs_touch_frame_t;

typedef struct psvs_touch_info_t {
    bool pad_down;
    int8_t pad_port;
    volatile int last; // Last frame index
    psvs_touch_frame_t frames[PSVS_TOUCH_MAX_FRAMES];
} psvs_touch_info_t;

#define PSVS_MOTION_MAX_FRAMES 64

typedef struct psvs_float3_t {
    float x;
    float y;
    float z;
} psvs_float3_t;

typedef struct psvs_motion_frame_t {
    uint64_t timestamp;
    uint32_t counter;
    psvs_float3_t gyro;
    psvs_float3_t accel;
} psvs_motion_frame_t;

typedef struct psvs_motion_info_t {
    uint32_t counter;
    volatile int last; // Last frame index
    psvs_motion_frame_t frames[PSVS_MOTION_MAX_FRAMES]; // All motion frames
} psvs_motion_info_t;

// Synthetic calibration handed to SceMotion. The raw samples injected into SceMotionDev are
// scaled with these values, so the two must always agree.
typedef struct psvs_motion_calib_t {
    SceFVector3 gyroZero;
    SceFVector3 gyroPosScale;
    SceFVector3 gyroNegScale;
    SceFVector3 accelZero;
    SceFVector3 accelPosScale;
    SceFVector3 accelNegScale;
} psvs_motion_calib_t;

typedef struct psvs_gamepad_t {
    psvs_gamepad_type_t type;
    uint64_t timestamp;
    unsigned int mac0;
    unsigned int mac1;
    psvs_touch_info_t touch;
    psvs_motion_info_t motion;
} psvs_gamepad_t;

static psvs_gamepad_t g_gamepad = {
    .type = PSVS_GAMEPAD_NONE,
};

#define PSVS_MOTION_GYRO_ZERO   0x8000
#define PSVS_MOTION_GYRO_SCALE  10.0f   // raw units per deg/sec (resolution = 0.1 deg/sec ~ 0.0009 rad/sec)
#define PSVS_MOTION_ACCEL_ZERO  0x8000
#define PSVS_MOTION_ACCEL_SCALE 1000.0f // raw units per G (resolution = 0.001 G)

static psvs_motion_calib_t g_motion_calib = {
    .gyroZero      = {PSVS_MOTION_GYRO_ZERO, PSVS_MOTION_GYRO_ZERO, PSVS_MOTION_GYRO_ZERO},
    .gyroPosScale  = {PSVS_MOTION_GYRO_SCALE, PSVS_MOTION_GYRO_SCALE, PSVS_MOTION_GYRO_SCALE},
    .gyroNegScale  = {PSVS_MOTION_GYRO_SCALE, PSVS_MOTION_GYRO_SCALE, PSVS_MOTION_GYRO_SCALE},
    .accelZero     = {PSVS_MOTION_ACCEL_ZERO, PSVS_MOTION_ACCEL_ZERO, PSVS_MOTION_ACCEL_ZERO},
    .accelPosScale = {PSVS_MOTION_ACCEL_SCALE, PSVS_MOTION_ACCEL_SCALE, PSVS_MOTION_ACCEL_SCALE},
    .accelNegScale = {PSVS_MOTION_ACCEL_SCALE, PSVS_MOTION_ACCEL_SCALE, PSVS_MOTION_ACCEL_SCALE},
};

// Pending HID read request of the bound gamepad. The buffer is filled asynchronously, so it is
// only read once the matching read event arrives (ds34motion 1.3.1 "delayed inputs" fix).
static SceUID g_bt_mutex_uid = -1;
static const unsigned char * g_bt_recv_buff = NULL;
static uint32_t g_bt_recv_length = 0;

// Force inline functions
#define INLINE __attribute__((always_inline)) __inline__

#define PSVS_BT_VID_SONY  0x054C
#define PSVS_BT_PID_DS3   0x0268
#define PSVS_BT_PID_DS4_1 0x05C4
#define PSVS_BT_PID_DS4_2 0x09CC

// The actual resolution is 1920x942, but the PSTV adds a ton of deadzone (TODO: make the deadzone optional)
#define PSVS_DS4_TOUCH_X (0 + 60)
#define PSVS_DS4_TOUCH_Y (0 + 120)
#define PSVS_DS4_TOUCH_W (1920 - 2*60)
#define PSVS_DS4_TOUCH_H (942 - 2*120)

#define PSVS_VITA_TOUCH_X 0
#define PSVS_VITA_TOUCH_Y 0
#define PSVS_VITA_TOUCH_W (960*2)
#define PSVS_VITA_TOUCH_H (544*2)

#define PSVS_VITA_TOUCH_BACK_X 0
#define PSVS_VITA_TOUCH_BACK_Y (54*2)
#define PSVS_VITA_TOUCH_BACK_W (960*2)
#define PSVS_VITA_TOUCH_BACK_H (445*2 - 54*2)

// DS4 raw motion units: one G is ~0x2000, one deg/sec is ~16 (0.064 deg/sec per unit)
#define PSVS_DS4_ACCEL_UNITS_PER_G   ((float) 0x2000)
#define PSVS_DS4_GYRO_DEG_PER_UNIT   0.064f

void psvs_bt_init() {
    g_bt_mutex_uid = ksceKernelCreateMutex("psvs_mutex_bt", 0, 0, NULL);
}

void psvs_bt_done() {
    if (g_bt_mutex_uid >= 0)
        ksceKernelDeleteMutex(g_bt_mutex_uid);
    g_bt_mutex_uid = -1;
}

INLINE static void _psvs_bt_lock() {
    if (g_bt_mutex_uid >= 0)
        ksceKernelLockMutex(g_bt_mutex_uid, 1, NULL);
}

INLINE static void _psvs_bt_unlock() {
    if (g_bt_mutex_uid >= 0)
        ksceKernelUnlockMutex(g_bt_mutex_uid, 1);
}

static psvs_gamepad_type_t _psvs_bt_get_gamepad_type(unsigned short vid, unsigned short pid) {
    // Check for DS3 and DS4 controllers
    if (vid == PSVS_BT_VID_SONY) {
        if (pid == PSVS_BT_PID_DS3)
            return PSVS_GAMEPAD_DS3;
        if (pid == PSVS_BT_PID_DS4_1 || pid == PSVS_BT_PID_DS4_2)
            return PSVS_GAMEPAD_DS4;
    }
    return PSVS_GAMEPAD_NONE;
}

static uint32_t _psvs_bt_get_report_size(psvs_gamepad_type_t type) {
    switch (type) {
        case PSVS_GAMEPAD_DS3: return sizeof(psvs_ds3_input_report_t);
        case PSVS_GAMEPAD_DS4: return sizeof(psvs_ds4_input_report_t);
        default: return 0;
    }
}

// Identify the gamepad behind a Bluetooth address (called on the connection event)
static psvs_gamepad_type_t _psvs_bt_identify(unsigned int mac0, unsigned int mac1) {
    unsigned short vid_pid[2] = {0, 0};
    int ret_vid_pid = ksceBtGetVidPid(mac0, mac1, vid_pid);

    psvs_gamepad_type_t type = PSVS_GAMEPAD_NONE;
    if (ret_vid_pid >= 0)
        type = _psvs_bt_get_gamepad_type(vid_pid[0], vid_pid[1]);

    // A DualShock 3 reports neither VID/PID nor a name over Bluetooth
    if (type == PSVS_GAMEPAD_NONE) {
        char name[0x79];
        int ret_name = ksceBtGetDeviceName(mac0, mac1, name);
        if ((unsigned int) ret_vid_pid == PSVS_BT_ERROR_NO_VID_PID && (unsigned int) ret_name == PSVS_BT_ERROR_NO_NAME)
            type = PSVS_GAMEPAD_DS3;
    }

    return type;
}

// Bind a gamepad and reset all captured data (calibration is kept, SceMotion only reads it once)
static void _psvs_bt_bind(psvs_gamepad_type_t type, unsigned int mac0, unsigned int mac1) {
    memset(&g_gamepad, 0, sizeof(g_gamepad));
    g_gamepad.type = type;
    g_gamepad.mac0 = mac0;
    g_gamepad.mac1 = mac1;
    g_gamepad.timestamp = ksceKernelGetSystemTimeWide();
    g_bt_recv_buff = NULL;
    g_bt_recv_length = 0;
}

static void _psvs_bt_unbind() {
    memset(&g_gamepad, 0, sizeof(g_gamepad));
    g_bt_recv_buff = NULL;
    g_bt_recv_length = 0;
}

INLINE static bool _psvs_bt_is_bound(unsigned int mac0, unsigned int mac1) {
    return g_gamepad.type != PSVS_GAMEPAD_NONE && g_gamepad.mac0 == mac0 && g_gamepad.mac1 == mac1;
}

INLINE static bool _psvs_bt_connection_timed_out() {
    return ksceKernelGetSystemTimeWide() - g_gamepad.timestamp > PSVS_BT_CONNECTION_TIMEOUT;
}

// Lazily bind the gamepad that is talking to us (fallback for a gamepad whose connection event was
// not seen, e.g. because it was already connected when this module started). Must hold the lock.
static bool _psvs_bt_connected(unsigned int mac0, unsigned int mac1) {

    // No gamepad is currently connected
    if (!g_gamepad.type || _psvs_bt_connection_timed_out()) {
        // Get VID and PID
        unsigned short result[2] = {0, 0};
        if (ksceBtGetVidPid(mac0, mac1, result) < 0)
            return false;
        // Get gamepad type
        psvs_gamepad_type_t type = _psvs_bt_get_gamepad_type(result[0], result[1]);
        // Initialize gamepad
        if (type)
            _psvs_bt_bind(type, mac0, mac1);
        return !! type;
    }

    // Connected gamepad
    if (_psvs_bt_is_bound(mac0, mac1)) {
        g_gamepad.timestamp = ksceKernelGetSystemTimeWide();
        return true;
    }

    // Not the connected gamepad
    return false;
}

static uint32_t _psvs_rescale_touch_x(int port, int32_t x) {
    x -= PSVS_DS4_TOUCH_X;
    x = (x < 0) ? 0 : x;
    x = (x >= PSVS_DS4_TOUCH_W) ? PSVS_DS4_TOUCH_W - 1 : x;
    if (port == SCE_TOUCH_PORT_FRONT)
        return (x * (PSVS_VITA_TOUCH_W - 1)) / (PSVS_DS4_TOUCH_W - 1) + PSVS_VITA_TOUCH_X;
    if (port == SCE_TOUCH_PORT_BACK)
        return (x * (PSVS_VITA_TOUCH_BACK_W - 1)) / (PSVS_DS4_TOUCH_W - 1) + PSVS_VITA_TOUCH_BACK_X;
    return 0;
}

static uint32_t _psvs_rescale_touch_y(int port, int32_t y) {
    y -= PSVS_DS4_TOUCH_Y;
    y = (y < 0) ? 0 : y;
    y = (y >= PSVS_DS4_TOUCH_H) ? PSVS_DS4_TOUCH_H - 1 : y;
    if (port == SCE_TOUCH_PORT_FRONT)
        return (y * (PSVS_VITA_TOUCH_H - 1)) / (PSVS_DS4_TOUCH_H - 1) + PSVS_VITA_TOUCH_Y;
    if (port == SCE_TOUCH_PORT_BACK)
        return (y * (PSVS_VITA_TOUCH_BACK_H - 1)) / (PSVS_DS4_TOUCH_H - 1) + PSVS_VITA_TOUCH_BACK_Y;
    return 0;
}

// Append a motion frame; raw values are in DS4 units (see PSVS_DS4_*)
static void _psvs_bt_push_motion_frame(int16_t gyro_x, int16_t gyro_y, int16_t gyro_z,
                                       int16_t accel_x, int16_t accel_y, int16_t accel_z) {
    psvs_motion_frame_t frame;

    // Convert angular velocity (degree / sec)
    frame.gyro.x = gyro_x * PSVS_DS4_GYRO_DEG_PER_UNIT;
    frame.gyro.y = gyro_y * PSVS_DS4_GYRO_DEG_PER_UNIT;
    frame.gyro.z = gyro_z * PSVS_DS4_GYRO_DEG_PER_UNIT;

    // Convert acceleration (G)
    frame.accel.x = accel_x / PSVS_DS4_ACCEL_UNITS_PER_G;
    frame.accel.y = accel_y / PSVS_DS4_ACCEL_UNITS_PER_G;
    frame.accel.z = accel_z / PSVS_DS4_ACCEL_UNITS_PER_G;

    // Update timestamp and counter
    frame.counter = ++ g_gamepad.motion.counter;
    frame.timestamp = g_gamepad.timestamp;

    // Append frame data
    int next = (g_gamepad.motion.last + 1) % PSVS_MOTION_MAX_FRAMES;
    g_gamepad.motion.frames[next] = frame;
    __atomic_store_n(&g_gamepad.motion.last, next, __ATOMIC_SEQ_CST);
}

static void _psvs_bt_process_ds4_report(const psvs_ds4_input_report_t * report) {

    // Handle touch events
    if (g_profile.bt_touch) {
        // Read touchpad press
        bool pressed = report->tpad && !g_gamepad.touch.pad_down;
        g_gamepad.touch.pad_down = report->tpad;

        // Toggle active port
        if (pressed && g_profile.bt_touch != PSVS_BT_TOUCH_FRONT) {
            if (g_gamepad.touch.pad_port < SCE_TOUCH_PORT_MAX_NUM - 1)
                ++ g_gamepad.touch.pad_port;
            else
                g_gamepad.touch.pad_port = (g_profile.bt_touch == PSVS_BT_TOUCH_TOGGLE_F_B) ? 0 : -1;
        }

        // Frame data
        psvs_touch_frame_t frame = {
            .timestamp = g_gamepad.timestamp,
            .port = g_gamepad.touch.pad_port,
            .count = 0,
        };

        // Add 1st finger
        if (!report->finger1_activelow) {
            psvs_touch_point_t point = {.id = report->finger1_id, .x = report->finger1_x, .y = report->finger1_y};
            frame.points[frame.count ++] = point;
        }
        // Add 2nd finger
        if (!report->finger2_activelow) {
            psvs_touch_point_t point = {.id = report->finger2_id, .x = report->finger2_x, .y = report->finger2_y};
            frame.points[frame.count ++] = point;
        }

        // Append frame data
        int next = (g_gamepad.touch.last + 1) % PSVS_TOUCH_MAX_FRAMES;
        g_gamepad.touch.frames[next] = frame;
        __atomic_store_n(&g_gamepad.touch.last, next, __ATOMIC_SEQ_CST);
    }

    // Handle motion events
    if (g_profile.bt_motion) {
        _psvs_bt_push_motion_frame(report->gyro_x, report->gyro_y, report->gyro_z,
                                   report->accel_x, report->accel_y, report->accel_z);
    }
}

static void _psvs_bt_process_ds3_report(const psvs_ds3_input_report_t * report) {

    // The DS3 has no touchpad, only motion
    if (g_profile.bt_motion) {
        // Map the DS3 sensors onto the DS4 axes and scale (same conversion as ds34motion).
        // The DS3 only has a yaw gyro, so the other two angular velocities stay zero.
        int16_t accel_x =  ((int16_t) report->accel_x) / 4;
        int16_t accel_y = -((int16_t) report->accel_z) / 4;
        int16_t accel_z = -((int16_t) report->accel_y) / 4;
        int16_t gyro_y  = (((int16_t) report->gyro_z) + 0x15FF) / 10;

        _psvs_bt_push_motion_frame(0, gyro_y, 0, accel_x, accel_y, accel_z);
    }
}

// A read request completed: the buffer now holds a fresh input report. Must hold the lock.
static void _psvs_bt_process_report(const unsigned char * buffer, uint32_t length) {
    uint32_t size = _psvs_bt_get_report_size(g_gamepad.type);
    if (!buffer || !size || length < size)
        return;

    if (g_gamepad.type == PSVS_GAMEPAD_DS4 && buffer[0] == PSVS_DS4_REPORT_ID) {
        g_gamepad.timestamp = ksceKernelGetSystemTimeWide();
        _psvs_bt_process_ds4_report((const psvs_ds4_input_report_t *) buffer);
    } else if (g_gamepad.type == PSVS_GAMEPAD_DS3 && buffer[0] == PSVS_DS3_REPORT_ID) {
        g_gamepad.timestamp = ksceKernelGetSystemTimeWide();
        _psvs_bt_process_ds3_report((const psvs_ds3_input_report_t *) buffer);
    }
}

void psvs_bt_on_hid_transfer(unsigned int mac0, unsigned int mac1, SceBtHidRequest * head) {
    _psvs_bt_lock();

    // Only follow gamepads while a Bluetooth feature is enabled, or a gamepad is already bound
    bool bound;
    if (g_profile.bt_touch || g_profile.bt_motion)
        bound = _psvs_bt_connected(mac0, mac1);
    else
        bound = _psvs_bt_is_bound(mac0, mac1);

    if (bound) {
        // Remember the buffer of the last read request in the chain. It is only read once the
        // corresponding read event arrives, because the transfer completes asynchronously.
        uint32_t size = _psvs_bt_get_report_size(g_gamepad.type);
        int steps = 0;
        for (SceBtHidRequest * request = head; request && steps < PSVS_BT_MAX_REQUEST_CHAIN; ++ steps) {
            if (request->type == PSVS_BT_HID_REQUEST_READ && request->buffer && request->length >= size) {
                g_bt_recv_buff = (const unsigned char *) request->buffer;
                g_bt_recv_length = request->length;
            }
            // ds34vita links its single read request to itself
            if (request->next == request)
                break;
            request = request->next;
        }
    }

    _psvs_bt_unlock();
}

void psvs_bt_on_read_event(const SceBtEvent * events, int count) {
    _psvs_bt_lock();

    for (int i = 0; i < count; ++ i) {
        const SceBtEvent * event = &events[i];

        switch (event->id) {
            case PSVS_BT_EVENT_CONNECTED:
                // Bind the first supported gamepad that connects
                if (!g_gamepad.type || _psvs_bt_connection_timed_out()) {
                    psvs_gamepad_type_t type = _psvs_bt_identify(event->mac0, event->mac1);
                    if (type)
                        _psvs_bt_bind(type, event->mac0, event->mac1);
                }
                break;

            case PSVS_BT_EVENT_DISCONNECTED:
                if (_psvs_bt_is_bound(event->mac0, event->mac1))
                    _psvs_bt_unbind();
                break;

            case PSVS_BT_EVENT_HID_READ:
                if (_psvs_bt_is_bound(event->mac0, event->mac1)) {
                    _psvs_bt_process_report(g_bt_recv_buff, g_bt_recv_length);
                    g_bt_recv_buff = NULL;
                    g_bt_recv_length = 0;
                }
                break;

            default:
                break;
        }
    }

    _psvs_bt_unlock();
}

int psvs_bt_touch_filter_input(bool peek, uint32_t port, SceTouchData *pData, uint32_t nBufs) {

    // Validate parameters
    if (port >= SCE_TOUCH_PORT_MAX_NUM || nBufs > 64)
        return SCE_TOUCH_ERROR_INVALID_ARG;

    // On connection dropped
    if (!g_gamepad.type || _psvs_bt_connection_timed_out()) {
        return nBufs;
    }

    // On very old data
    if (ksceKernelGetSystemTimeWide() - g_gamepad.timestamp > PSVS_BT_PACKET_TIMEOUT) {
        port = SCE_TOUCH_PORT_MAX_NUM + 1; // Make both panels inactive
    }

    // Get latest frame
    int last = __atomic_load_n(&g_gamepad.touch.last, __ATOMIC_SEQ_CST); // Atomic ensures that we never read the buffer that is currently written
    psvs_touch_frame_t * frame = &g_gamepad.touch.frames[last];

    // Base data
    SceTouchData data = {
        .timeStamp = frame->timestamp,
        .status = 0,
        .reportNum = 0,
    };

    // On active panel
    if (port == frame->port) {
        data.reportNum = frame->count;
        for (int j = 0; j < frame->count; ++ j) {
            SceTouchReport point = {
                .id = frame->points[j].id,
                .force = 128,
                .x = _psvs_rescale_touch_x(port, frame->points[j].x),
                .y = _psvs_rescale_touch_y(port, frame->points[j].y),
            };
            data.report[j] = point;
        }
    }

    // TODO: use more then one frame
    for (int i = 0; i < nBufs; ++ i) {
        // Override data
        pData[i] = data;
    }

    // Return touch data
    return nBufs;
}

bool psvs_bt_motion_available() {

    // No gamepad, or no motion data captured yet
    if (!g_gamepad.type || !g_gamepad.motion.counter)
        return false;

    // Connection dropped, or last frame is too old
    int last = __atomic_load_n(&g_gamepad.motion.last, __ATOMIC_SEQ_CST);
    uint64_t now = ksceKernelGetSystemTimeWide();
    if (now - g_gamepad.timestamp > PSVS_BT_CONNECTION_TIMEOUT)
        return false;
    if (now - g_gamepad.motion.frames[last].timestamp > PSVS_BT_PACKET_TIMEOUT)
        return false;

    return true;
}

int psvs_bt_motion_filter_read(SceMotionDevResult * resultList, uint32_t count, int * setFlag) {

    // Kernel side data buffer
    SceMotionDevResult buffer;
    memset(&buffer, 0, sizeof(buffer));

    // Count is always 64, but it does not hurt to check
    if (count == 0)
        return count;

    // Get latest frame
    int last = __atomic_load_n(&g_gamepad.motion.last, __ATOMIC_SEQ_CST); // Atomic ensures that we never read the buffer that is currently written
    psvs_motion_frame_t frame = g_gamepad.motion.frames[last];

    // Fill out buffer
    buffer.timestamp = frame.timestamp;
    buffer.entryCount = 1;
    buffer.magnCalibIndex = 0;
    buffer.magnFieldStab = 0;
    buffer.gyroCalibIndex = 0;
    buffer.timeInMSec = ksceKernelGetSystemTimeWide() / 1000;

    // Fill out entry
    buffer.entryList[0].flags = SCE_MOTION_DEV_ENTRY_HAS_GYRO_DATA | SCE_MOTION_DEV_ENTRY_HAS_ACCEL_DATA;

    // Scale acceleration
    frame.accel.x *= (frame.accel.x > 0.0f ? g_motion_calib.accelPosScale.x : g_motion_calib.accelNegScale.x);
    frame.accel.y *= (frame.accel.y > 0.0f ? g_motion_calib.accelPosScale.y : g_motion_calib.accelNegScale.y);
    frame.accel.z *= (frame.accel.z > 0.0f ? g_motion_calib.accelPosScale.z : g_motion_calib.accelNegScale.z);

    // Scale gyro
    frame.gyro.x *= (frame.gyro.x > 0.0f ? g_motion_calib.gyroPosScale.x : g_motion_calib.gyroNegScale.x);
    frame.gyro.y *= (frame.gyro.y > 0.0f ? g_motion_calib.gyroPosScale.y : g_motion_calib.gyroNegScale.y);
    frame.gyro.z *= (frame.gyro.z > 0.0f ? g_motion_calib.gyroPosScale.z : g_motion_calib.gyroNegScale.z);

    // Apply orientation (accel_y and accel_x are swapped by SceMotion)
    if (g_profile.bt_motion == PSVS_BT_MOTION_NORMAL) {
        // Convert acceleration to uint16_t with offset
        buffer.entryList[0].accel_y = (uint16_t) (0.5f + frame.accel.x + g_motion_calib.accelZero.x);
        buffer.entryList[0].accel_x = (uint16_t) (0.5f + frame.accel.z + g_motion_calib.accelZero.z);
        buffer.entryList[0].accel_z = (uint16_t) (0.5f + frame.accel.y + g_motion_calib.accelZero.y);

        // Convert gyro to uint16_t with offset
        buffer.entryList[0].gyro_y = (uint16_t) (0.5f + frame.gyro.x + g_motion_calib.gyroZero.x);
        buffer.entryList[0].gyro_x = (uint16_t) (0.5f + frame.gyro.z + g_motion_calib.gyroZero.z);
        buffer.entryList[0].gyro_z = (uint16_t) (0.5f + frame.gyro.y + g_motion_calib.gyroZero.y);
    } else {
        // Convert acceleration to uint16_t with offset
        buffer.entryList[0].accel_y = (uint16_t) (0.5f + frame.accel.x + g_motion_calib.accelZero.x);
        buffer.entryList[0].accel_x = (uint16_t) (0.5f - frame.accel.y + g_motion_calib.accelZero.y);
        buffer.entryList[0].accel_z = (uint16_t) (0.5f + frame.accel.z + g_motion_calib.accelZero.z);

        // Convert gyro to uint16_t with offset
        buffer.entryList[0].gyro_y = (uint16_t) (0.5f + frame.gyro.x + g_motion_calib.gyroZero.x);
        buffer.entryList[0].gyro_x = (uint16_t) (0.5f - frame.gyro.y + g_motion_calib.gyroZero.y);
        buffer.entryList[0].gyro_z = (uint16_t) (0.5f + frame.gyro.z + g_motion_calib.gyroZero.z);
    }

    // Copy to user buffer
    ksceKernelMemcpyKernelToUser(resultList, &buffer, sizeof(buffer));

    // For now only a single event for each call
    return 1;
}

int psvs_bt_motion_reset_device_info(uint32_t * info) {
    // Set device info
    *info = 0;
    // Return SCE_OK
    return 0;
}

int psvs_bt_motion_reset_gyro_bias(SceMotionDevGyroBias * bias) {
    // Reset gyro zero
    g_motion_calib.gyroZero.x = PSVS_MOTION_GYRO_ZERO;
    g_motion_calib.gyroZero.y = PSVS_MOTION_GYRO_ZERO;
    g_motion_calib.gyroZero.z = PSVS_MOTION_GYRO_ZERO;
    // Set gyro bias
    bias->reserved = 0;
    bias->zero = g_motion_calib.gyroZero;
    // Return SCE_OK
    return 0;
}

int psvs_bt_motion_reset_gyro_calib_data(SceMotionDevGyroCalibData * data) {
    // Reset gyro scale
    g_motion_calib.gyroZero.x = PSVS_MOTION_GYRO_ZERO;
    g_motion_calib.gyroZero.y = PSVS_MOTION_GYRO_ZERO;
    g_motion_calib.gyroZero.z = PSVS_MOTION_GYRO_ZERO;
    g_motion_calib.gyroPosScale.x = PSVS_MOTION_GYRO_SCALE;
    g_motion_calib.gyroPosScale.y = PSVS_MOTION_GYRO_SCALE;
    g_motion_calib.gyroPosScale.z = PSVS_MOTION_GYRO_SCALE;
    g_motion_calib.gyroNegScale.x = PSVS_MOTION_GYRO_SCALE;
    g_motion_calib.gyroNegScale.y = PSVS_MOTION_GYRO_SCALE;
    g_motion_calib.gyroNegScale.z = PSVS_MOTION_GYRO_SCALE;
    // Set gyro calib data
    data->zero = g_motion_calib.gyroZero;
    data->xPosScale = 0x8000 * 0.1f;
    data->yPosScale = 0x8000 * 0.1f;
    data->zPosScale = 0x8000 * 0.1f;
    data->xPos.x = 0x10000;
    data->xPos.y = 0x8000;
    data->xPos.z = 0x8000;
    data->yPos.x = 0x8000;
    data->yPos.y = 0x10000;
    data->yPos.z = 0x8000;
    data->zPos.x = 0x8000;
    data->zPos.y = 0x8000;
    data->zPos.z = 0x10000;
    data->xNegScale = -0x8000 * 0.1f;
    data->yNegScale = -0x8000 * 0.1f;
    data->zNegScale = -0x8000 * 0.1f;
    data->xNeg.x = 0;
    data->xNeg.y = 0x8000;
    data->xNeg.z = 0x8000;
    data->yNeg.x = 0x8000;
    data->yNeg.y = 0;
    data->yNeg.z = 0x8000;
    data->zNeg.x = 0x8000;
    data->zNeg.y = 0x8000;
    data->zNeg.z = 0;
    // Return SCE_OK
    return 0;
}

int psvs_bt_motion_reset_accel_calib_data(SceMotionDevAccCalibData * data) {
    // Reset accel scale
    g_motion_calib.accelZero.x = PSVS_MOTION_ACCEL_ZERO;
    g_motion_calib.accelZero.y = PSVS_MOTION_ACCEL_ZERO;
    g_motion_calib.accelZero.z = PSVS_MOTION_ACCEL_ZERO;
    g_motion_calib.accelPosScale.x = PSVS_MOTION_ACCEL_SCALE;
    g_motion_calib.accelPosScale.y = PSVS_MOTION_ACCEL_SCALE;
    g_motion_calib.accelPosScale.z = PSVS_MOTION_ACCEL_SCALE;
    g_motion_calib.accelNegScale.x = PSVS_MOTION_ACCEL_SCALE;
    g_motion_calib.accelNegScale.y = PSVS_MOTION_ACCEL_SCALE;
    g_motion_calib.accelNegScale.z = PSVS_MOTION_ACCEL_SCALE;
    // Set accel calib data
    data->xPosScale = 0x8000 * 0.001f;
    data->yPosScale = 0x8000 * 0.001f;
    data->zPosScale = 0x8000 * 0.001f;
    data->xPos.x = 0x10000;
    data->xPos.y = 0x8000;
    data->xPos.z = 0x8000;
    data->yPos.x = 0x8000;
    data->yPos.y = 0x10000;
    data->yPos.z = 0x8000;
    data->zPos.x = 0x8000;
    data->zPos.y = 0x8000;
    data->zPos.z = 0x10000;
    data->xNegScale = -0x8000 * 0.001f;
    data->yNegScale = -0x8000 * 0.001f;
    data->zNegScale = -0x8000 * 0.001f;
    data->xNeg.x = 0;
    data->xNeg.y = 0x8000;
    data->xNeg.z = 0x8000;
    data->yNeg.x = 0x8000;
    data->yNeg.y = 0;
    data->yNeg.z = 0x8000;
    data->zNeg.x = 0x8000;
    data->zNeg.y = 0x8000;
    data->zNeg.z = 0;
    // Return SCE_OK
    return 0;
}
