#ifndef PACKET
#define PACKET

#define MAX_PACKET_LEN 1000

typedef struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
}packet;

int get_packet_length(struct packet pk){
    int packet_length = strlen(pk.filename);
    packet_length += snprintf(NULL, 0, "%d", pk.size);
    packet_length += snprintf(NULL, 0, "%d", pk.frag_no);
    packet_length += snprintf(NULL, 0, "%d",pk.total_frag);
    packet_length += 4;
	return packet_length;
}

#endif /* !PACKET */
