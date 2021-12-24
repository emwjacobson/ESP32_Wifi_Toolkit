#include <stdint.h>

typedef struct {
    uint32_t magic_number;   /* magic number: 0xa1b2c3d4 */
    uint16_t version_major;  /* major version number: 2 */
    uint16_t version_minor;  /* minor version number: 4 */
    int32_t  thiszone;       /* GMT to local correction: 0 */
    uint32_t sigfigs;        /* accuracy of timestamps: 0 */
    uint32_t snaplen;        /* max length of captured packets, in octets: 65535 */
    uint32_t network;        /* data link type: 105(?) */
} pcap_hdr_t;

// https://wiki.wireshark.org/Development/LibpcapFileFormat
typedef struct {
    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

void capture_init() {
    
}

void capture_start() {

}

void capture_stop() {

}
