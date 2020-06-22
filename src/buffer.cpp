#include <buffer.h>

/*******************************************************
 * VARIABLES
 *******************************************************/

char _buff[MAX_BUFFER]; // buffer

/*******************************************************
 * BUFFER FUNCTIONS
 *******************************************************/

char* getBuffer(void)
{
  return _buff;
}

/**
 * @brief clear buffer
 * 
 * @param all set all bytes to zero
 */
void clearBuff(bool all)
{
  // _buff_end = 0;
  if (all)
    memset(_buff, 0, MAX_BUFFER);
  else
    _buff[0] = 0x00;
}

/**
 * @brief get number after target string
 * 
 * @param target string to find
 * @return int16_t number after string
 */
int16_t getIntAfterStr(const char* target)
{
  char* p;
  if ((p = strstr(_buff, target)) == NULL)
    return -99;

  return (int16_t)atoi(p + strlen(target));
}

/**
 * @brief get number after target string,
 *        starting after start string
 * 
 * @param target string to find
 * @return int16_t number after string
 */
int16_t getIntAfterStartStr(const char* start, const char* target)
{
  char* p;
  if ((p = strstr(_buff, start)) == NULL)
    return -99;

  if ((p = strstr(p, target)) == NULL)
    return -99;

  return (int16_t)atoi(p + strlen(target));
}
