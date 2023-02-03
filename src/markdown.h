#ifndef MARKDOWN
#define MARKDOWN

#include <string>
struct ParseContext;

namespace markdown
{
  std::string markdownToHtml(char* sourceStart, char* sourceEnd);
}

#endif  //MARKDOWN
