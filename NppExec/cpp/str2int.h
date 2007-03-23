/*******************************************************
 *
 *  str2int ver 1.0.1
 *  --------------------------------
 *  (C) DV, 13 December 2006
 *  --------------------------------
 *
 *  str2int converts a string into an integer
 *  (dec, hex, bin, oct)
 *
 *******************************************************/

// >>>>>>>>>>>>>>>>>>>>>>>>>>>> str2int >>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>
// >>>> usage:
// >>>>   str2int<int, char>("-1235.890") -> -1235    (DEC, signed)
// >>>>   str2int<int, char>("-0xFAE37")  -> -1027639 (HEX, signed)
// >>>>   str2int<int, char>("0b1001110)  ->  78      (BIN, signed)
// >>>>   str2int<int, char>("053162")    ->  22130   (OCT, signed)
// >>>>   str2int<int, char>("$FAE37")    ->  1027639 (HEX, signed)
// >>>>

#ifndef _str_to_int_t_h_
#define _str_to_int_t_h_
//-----------------------------------------------------------------------------

template <class T> static int str_safe_length(const T* str)
{
  if (str != NULL) {
    T* p = (T*) str;
    while (*p) {
      p++;
    }
    return ( (int) (p - str) );
  }
  return 0;
}

//-----------------------------------------------------------------------------

template<class I, class T> static I char2int(const T ch)
{
  I result = 0;

  if (ch >= '0' && ch <= '9')
    result = ch - '0';
  else if (ch >= 'A' && ch <= 'Z')
    result = 10 + (ch - 'A');
  else if (ch >= 'a' && ch <= 'z')
    result = 10 + (ch - 'a');

  return result;
}

template<class T> inline static T latin_upcase(T ch)
{
  if (ch >= 'a' && ch <= 'z')
  {
    ch += ('A' - 'a');
  }
  return ch;
}

template<class T> static int find_wrongsymb_pos(const T* str, int len, int base)
{
  int i = 0;
  T   ch;
  T   max_ch;

  max_ch = 'A' + (base-11);
  while (i < len)
  {
    ch = str[i];    
    if (ch < '0' || ch > '9')
    {
      ch = latin_upcase(ch);
      if (ch < 'A' || ch > max_ch)
        break;
    }
    i++;
  }
  return i;
}

template<class T> static int find_wrongint_pos(const T* str, int len, int base)
{
  int i = 0;
  T   ch;
  T   max_ch;

  max_ch = '0' + (base-1);
  while (i < len)
  {
    ch = str[i];    
    if (ch < '0' || ch > max_ch)
      break;
    i++;
  }
  return i;
}

template<class I, class T> static I base2int(const T* str, int len, int base)
{
  int i;
  I   result = 0;
  I   digit_multiplier;
  I   digit_value;
  
  if (str == NULL || len == 0)
    return 0;

  if (base <= 10)
    len = find_wrongint_pos(str, len, base);
  else
    len = find_wrongsymb_pos(str, len, base); 

  if (len == 0)
    return 0;

  digit_multiplier = 1;
  for (i = len-1; i >= 0; i--)
  {
    digit_value = char2int<I, T>(str[i])*digit_multiplier;
    digit_multiplier *= base;
    result += digit_value;
  }

  return result;
}

template<class I, class T> I str2int(const T* str, bool bSigned=true)
{
  int len;
  I   result = 0;
  T*  p;

  len = str_safe_length(str);
  if (len == 0)
    return 0;

  p = (T*) str;
  if (str[0] == '-')
  {
    p++;
    len--;
  }
  else if (str[0] == '+')
  {
    p++;
    len--;
    bSigned = false;
  }

  if (len >= 2 && p[0] == '0')
  {
    switch (p[1])
    {
      case 'x': // 0x...
      {
        // HEX value
        result = base2int<I, T>(p+2, len-2, 16);
        break;
      }
      case 'b': // 0b...
      {
        // BIN value
        result = base2int<I, T>(p+2, len-2, 2);
        break;
      }
      case '0': // 00...
      {
        // DEC value with leading zeros
        result = base2int<I, T>(p+2, len-2, 10);
        break;
      }
      default: // 0...
      {
        // OCT value
        result = base2int<I, T>(p+1, len-1, 8);
      }
    }
  }
  else if (len >= 1 && p[0] == '$')
  {
    // HEX value
    result = base2int<I, T>(p+1, len-1, 16);
  }
  else
  {
    // DEC value
    result = base2int<I, T>(p, len, 10);
  }
  
  if (bSigned && p > str)
    result = -result;
  
  return result;
}

//-----------------------------------------------------------------------------
#endif

