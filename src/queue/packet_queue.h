#ifndef _AVPLAYERWIDGET_PACKET_QUEUE_H_
#define _AVPLAYERWIDGET_PACKET_QUEUE_H_

extern "C"
{
#include "libavcodec/avcodec.h"
#include "SDL2/SDL.h"
}

/* queue size */
#define DEF_PKTQ_SIZE       (40 * 1024 * 1024)   // 50MB
#define MAX_PKTQ_SIZE       (1024 * 1024 * 1024) // 1GB
#define MIN_PKTQ_SIZE       (10 * 1024 * 1024)   // 10MB

class FrameQueue;

/* packet node */
typedef struct Packet {
    struct Packet       *next;     // pointer of next node
    AVPacket            *pkt;      // pointer of packet
}Packet;

/* packet queue */
class PacketQueue {
private:
    friend class FrameQueue;

private:
    Packet *            head;     // head of queue
    Packet *            tail;     // tail of queue
    int32_t             len;       // length of queue
    int64_t             size;      // size of queue in byte
    int64_t             duration;  // duration (unit: second)
    bool                abort_req; // abort requestion
    bool                read_eof;  // whether read eof
    SDL_mutex *         mutex;    // mutex
    SDL_cond *          cond;     // cond

public:
    PacketQueue ();
	~PacketQueue ();
	int       init         ();
	int       put          (AVPacket *pkt);
	AVPacket *get          ();
	void      set_read_eof (bool is_read_eof);
	void      restore      ();
	void      abort        ();
	void      clear        ();
	int       get_len      ();
	int64_t   get_size     ();
	int64_t   get_duration ();
    bool      is_eof       ();
};

#endif /* _AVPLAYERWIDGET_PACKET_QUEUE_H_ */