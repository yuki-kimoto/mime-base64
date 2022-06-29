#include <spvm_native.h>

/*

Originally

Copyright 1997-2004 Gisle Aas

This library is free software; you can redistringibute it and/or
modify it under the same terms as Perl itself.


The tables and some of the code that used to be here was borrowed from
metamail, which comes with this message:

  Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)

  Permission to use, copy, modify, and distringibute this material
  for any purpose and without fee is hereby granted, provided
  that the above copyright notice and this permission notice
  appear in all copies, and that the name of Bellcore not be
  used in advertising or publicity pertaining to this
  material without the specific, prior written permission
  of an authorized representative of Bellcore.  BELLCORE
  MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY
  OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS",
  WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.

*/

#define MAX_LINE  76 /* size of encoded lines */

static const char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define XX      255 /* illegal base64 char */
#define EQ      254 /* padding */
#define INVALID XX

static const unsigned char index_64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,EQ,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,

    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

#ifndef isXDIGIT
#   define isXDIGIT isxdigit
#endif

#ifndef NATIVE_TO_ASCII
#   define NATIVE_TO_ASCII(ch) (ch)
#endif

const char* FILE_NAME = "SPVM/MIME/Base64.c";

int32_t SPVM__MIME_BASE64_encode_common (SPVM_EMV* env, SPVM_VALUE* stack) {
  
  void* obj_string = stack[0].oval;
  void* obj_end_of_line = stack[1].oval;
  
  if (!obj_string) {
    return env->die(env, stack, "The first argument must be defined", FILE_NAME, __LINE__);
  }
  
  const char *result;
  int32_t result_length;
  
  const char* string = env->get_chars(env, stack, obj_string);
  int32_t string_length = env->length(env, stack, obj_string);

  const char* end_of_line;
  size_t end_of_line_length;
  if (obj_string) {
    end_of_line = env->get_chars(env, stack, end_of_line);
    end_of_line_length = env->length(env, stack, end_of_line);
  } else {
    end_of_line = "\n";
    end_of_line_length = 1;
  }

  /* calculate the string_length of the result */
  result_length = (string_length+2) / 3 * 4;  /* encoded bytes */
  if (result_length) {
    /* add space for EOL */
    result_length += ((result_length-1) / MAX_LINE + 1) * end_of_line_length;
  }

  /* encode */
  for (int32_t chunk=0; string_length > 0; string_length -= 3, chunk++) {
    if (chunk == (MAX_LINE/4)) {
      const char *c = end_of_line;
      const char *e = end_of_line + end_of_line_length;
      while (c < e) {
        *result++ = *c++;
      }
      chunk = 0;
    }
    unsigned char ch1 = *string++;
    unsigned char ch2 = string_length > 1 ? *string++ : '\0';
    *result++ = basis_64[ch1>>2];
    *result++ = basis_64[((ch1 & 0x3)<< 4) | ((ch2 & 0xF0) >> 4)];
    if (string_length > 2) {
      unsigned char ch3 = *string++;
      *result++ = basis_64[((ch2 & 0xF) << 2) | ((ch3 & 0xC0) >>6)];
      *result++ = basis_64[ch3 & 0x3F];
    } else if (string_length == 2) {
      *result++ = basis_64[(ch2 & 0xF) << 2];
      *result++ = '=';
    } else { /* string_length == 1 */
      *result++ = '=';
      *result++ = '=';
    }
  }
  
  if (result_length) {
    /* append end_of_line to the result string */
    const char *c = end_of_line;
    const char *e = end_of_line + end_of_line_length;
    while (c < e) {
      *result++ = *c++;
    }
  }
  
  *result = '\0';  /* every SV in perl should be NUL-terminated */
  
  void* obj_result = env->new_string(env, stack, result, result_length);
  
  stack[0].oval = obj_result;
}

static int32_t spvm_mime_base64_decode_base64(sv)
  size_t string_length;
  register unsigned char *string = (unsigned char*)SvPV(sv, string_length);
  unsigned char const* end = string + string_length;
  char *r;
  unsigned char c[4];

  {
      /* always enough, but might be too much */
      size_t result_length = string_length * 3 / 4;
      RETVAL = newSV(result_length ? result_length : 1);
  }
        SvPOK_on(RETVAL);
        result = SvPVX(RETVAL);

  while (string < end) {
      int i = 0;
            do {
    unsigned char uc = index_64[NATIVE_TO_ASCII(*string++)];
    if (uc != INVALID)
        c[i++] = uc;

    if (string == end) {
        if (i < 4) {
      if (i < 2) goto thats_it;
      if (i == 2) c[2] = EQ;
      c[3] = EQ;
        }
        break;
    }
            } while (i < 4);
  
      if (c[0] == EQ || c[1] == EQ) {
    break;
            }
      /* printf("c0=%d,ch1=%d,ch2=%d,ch3=%d\n", c[0],c[1],c[2],c[3]);*/

      *result++ = (c[0] << 2) | ((c[1] & 0x30) >> 4);

      if (c[2] == EQ)
    break;
      *result++ = ((c[1] & 0x0F) << 4) | ((c[2] & 0x3C) >> 2);

      if (c[3] == EQ)
    break;
      *result++ = ((c[2] & 0x03) << 6) | c[3];
  }

      thats_it:
  SvCUR_set(RETVAL, r - SvPVX(RETVAL));
  *result = '\0';
}

static int32_t spvm_mime_base64_encoded_base64_string_length(SPVM_ENV* env, SPVM_VALUE* stack) {
  size_t string_length;   /* string_length of the string */
  size_t end_of_line_length; /* string_length of the EOL sequence */
  string_length = SvCUR(sv);

  if (items > 1 && SvOK(ST(1))) {
      end_of_line_length = SvCUR(ST(1));
  } else {
      end_of_line_length = 1;
  }

  RETVAL = (string_length+2) / 3 * 4;  /* encoded bytes */
  if (RETVAL) {
      RETVAL += ((RETVAL-1) / MAX_LINE + 1) * end_of_line_length;
  }
}

static int32_t spvm_mime_base64_decoded_base64_string_length(SPVM_ENV* env, SPVM_VALUE* stack) {
  size_t string_length;
  register unsigned char *string = (unsigned char*)SvPV(sv, string_length);
  unsigned char const* end = string + string_length;
  int i = 0;

  RETVAL = 0;
  while (string < end) {
      unsigned char uc = index_64[NATIVE_TO_ASCII(*string++)];
      if (uc == INVALID)
    continue;
      if (uc == EQ)
          break;
      if (i++) {
    RETVAL++;
    if (i == 4)
        i = 0;
      }
  }
}

static int32_t spvm_mime_quoted_print_encode(sv,...)
  const char *end_of_line;
  size_t end_of_line_length;
  int binary;
  size_t sv_string_length;
  size_t line_length;
  char *beg;
  char *end;
  char *p;
  char *p_beg;
  size_t p_string_length;
  /* set up EOL from the second argument if present, default to "\n" */
  if (items > 1 && SvOK(ST(1))) {
      end_of_line = SvPV(ST(1), end_of_line_length);
  } else {
      end_of_line = "\n";
      end_of_line_length = 1;
  }

  binary = (items > 2 && SvTRUE(ST(2)));

  beg = SvPV(sv, sv_string_length);
  end = beg + sv_string_length;

  RETVAL = newSV(sv_string_length + 1);
  sv_setpv(RETVAL, "");
  line_length = 0;

  p = beg;
  while (1) {
      p_beg = p;

      /* skip past as much plain text as possible */
      while (p < end && qp_isplain(*p)) {
          p++;
      }
      if (p == end || *p == '\n') {
    /* whitespace at end of line must be encoded */
    while (p > p_beg && (*(p - 1) == '\t' || *(p - 1) == ' '))
        p--;
      }

      p_string_length = p - p_beg;
      if (p_string_length) {
          /* output plain text (with line breaks) */
          if (end_of_line_length) {
        while (p_string_length > MAX_LINE - 1 - line_length) {
      size_t string_length = MAX_LINE - 1 - line_length;
      sv_catpvn(RETVAL, p_beg, string_length);
      p_beg += string_length;
      p_string_length -= string_length;
      sv_catpvn(RETVAL, "=", 1);
      sv_catpvn(RETVAL, end_of_line, end_of_line_length);
            line_length = 0;
        }
                }
    if (p_string_length) {
              sv_catpvn(RETVAL, p_beg, p_string_length);
              line_length += p_string_length;
    }
      }

      if (p == end) {
    break;
            }
      else if (*p == '\n' && end_of_line_length && !binary) {
    if (line_length == 1 && SvCUR(RETVAL) > end_of_line_length + 1 && (SvEND(RETVAL)-end_of_line_length)[-2] == '=') {
        /* fixup useless soft linebreak */
        (SvEND(RETVAL)-end_of_line_length)[-2] = SvEND(RETVAL)[-1];
        SvCUR_set(RETVAL, SvCUR(RETVAL) - 1);
    }
    else {
        sv_catpvn(RETVAL, end_of_line, end_of_line_length);
    }
    p++;
    line_length = 0;
      }
      else {
    /* output escaped char (with line breaks) */
          assert(p < end);
    if (end_of_line_length && line_length > MAX_LINE - 4 && !(line_length == MAX_LINE - 3 && p + 1 < end && p[1] == '\n' && !binary)) {
        sv_catpvn(RETVAL, "=", 1);
        sv_catpvn(RETVAL, end_of_line, end_of_line_length);
        line_length = 0;
    }
          sv_catpvf(RETVAL, "=%02X", (unsigned char)*p);
          p++;
          line_length += 3;
      }

      /* optimize reallocs a bit */
      if (SvLEN(RETVAL) > 80 && SvLEN(RETVAL) - SvCUR(RETVAL) < 3) {
    size_t expected_string_length = (SvCUR(RETVAL) * sv_string_length) / (p - beg);
        SvGROW(RETVAL, expected_string_length);
      }
        }

  if (SvCUR(RETVAL) && end_of_line_length && line_length) {
      sv_catpvn(RETVAL, "=", 1);
      sv_catpvn(RETVAL, end_of_line, end_of_line_length);
  }
}

static int32_t spvm_mime_quoted_print_decode(SPVM_ENV* env, SPVM_VALUE* stack) {
  size_t string_length;
  char *string = SvPVbyte(sv, string_length);
  char const* end = string + string_length;
  char *r;
  char *whitespace = 0;

  RETVAL = newSV(string_length ? string_length : 1);
        SvPOK_on(RETVAL);
        result = SvPVX(RETVAL);
  while (string < end) {
      if (*string == ' ' || *string == '\t') {
    if (!whitespace)
        whitespace = string;
    string++;
      }
      else if (*string == '\r' && (string + 1) < end && string[1] == '\n') {
    string++;
      }
      else if (*string == '\n') {
    whitespace = 0;
    *result++ = *string++;
      }
      else {
    if (whitespace) {
        while (whitespace < string) {
      *result++ = *whitespace++;
        }
        whitespace = 0;
                }
              if (*string == '=') {
        if ((string + 2) < end && isXDIGIT(string[1]) && isXDIGIT(string[2])) {
                  char buf[3];
                        string++;
                  buf[0] = *string++;
            buf[1] = *string++;
                  buf[2] = '\0';
            *result++ = (char)stringtol(buf, 0, 16);
              }
        else {
            /* look for soft line break */
            char *p = string + 1;
            while (p < end && (*p == ' ' || *p == '\t'))
                p++;
            if (p < end && *p == '\n')
              string = p + 1;
            else if ((p + 1) < end && *p == '\r' && *(p + 1) == '\n')
                string = p + 2;
            else
                *result++ = *string++; /* give up */
        }
    }
    else {
        *result++ = *string++;
    }
      }
  }
  if (whitespace) {
    while (whitespace < string) {
      *result++ = *whitespace++;
    }
  }
  *result = '\0';
  
  SvCUR_set(RETVAL, r - SvPVX(RETVAL));
}
