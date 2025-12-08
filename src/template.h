#ifndef SLAB_TEMPLATE_H
#define SLAB_TEMPLATE_H

#include <stdx_string.h>
#include <stdx_strbuilder.h>

typedef struct MISsgResult
{
  int       error_code;    // 0 = sucess, 1 = error
  int       error_line;    // error line
  int       error_column;  // error column
  XSmallstr error_message; // error message
} MISsgResult;

MISsgResult ssg_expand_minima_template(XSlice input, XStrBuilder* out);

#endif //SLAB_TEMPLATE_H

