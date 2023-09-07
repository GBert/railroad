/* Contributed by Rainer Müller */

#ifndef _DECODER_Z21_H_
#define _DECODER_Z21_H_

void z21_conf_info(unsigned char *data, int datsize);
void z21_comm_ext(char *timestamp, int source, unsigned char *data, int datsize);

#endif /* _DECODER_Z21_H_ */
