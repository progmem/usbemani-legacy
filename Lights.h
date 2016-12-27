#ifndef _LIGHTS_H_
#define _LIGHTS_H_

void Lights_Init(void);
void Lights_StoreState(uint16_t OutputData);
uint16_t Lights_RetrieveState(void);

#endif