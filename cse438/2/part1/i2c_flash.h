#ifndef I2C_FLASH_H
#define I2C_FLASH_H
#define FLASH_KEY 'k'

#define FLASHGETS   _IOR(FLASH_KEY, 0, int *)
#define FLASHGETP   _IOR(FLASH_KEY, 1, int *)
#define FLASHSETP   _IOW(FLASH_KEY, 2, int)
#define FLASHERASE  _IO(FLASH_KEY, 3)

#endif
