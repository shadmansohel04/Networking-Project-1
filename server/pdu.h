#ifndef PDU_H
#define PDU_H

#define MAX_DATA_LEN 100

typedef struct {
  char type;
  char data[MAX_DATA_LEN];
} PDU;

#endif /* PDU_H */
