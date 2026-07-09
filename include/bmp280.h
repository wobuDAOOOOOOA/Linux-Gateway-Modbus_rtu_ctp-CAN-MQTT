#ifndef BMP280_H
#define BMP280_H

void bmp280_calc_data(u_int32_t raw_press, u_int32_t raw_temp, float *temp, float *press);
int BMP280_READ_Init(void);
int BMP280_READ(void);
int BMP280_READ_close(void);


#endif