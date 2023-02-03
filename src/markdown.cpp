#include "markdown.h"
#include "parser_utils.h"
#include <vector>

namespace markdown
{
  struct MDToken
  {
    enum Type
    {
      TOKEN_PARAGRAPH,
      TOKEN_H1,
      TOKEN_H2,
      TOKEN_H3,
      TOKEN_H4,
      TOKEN_H5,
      TOKEN_H6,
      TOKEN_SPACE,
      TOKEN_ENDL,
    };

    Type type;
    char* start;
    char* end;
  };

  struct MDNode
  {
    std::string text;
    virtual std::string toString() { return text; }
    MDNode(std::string text) : text(text) { }
  };

  struct MDContainerNode : MDNode
  {
    std::vector<MDNode> children;
    MDContainerNode(std::string text) : MDNode(text) { }
    MDContainerNode() : MDNode("") { }

    void addChildNode(MDNode& node)
    {
      children.push_back(node);
    }
  };

  struct MDHeaderNode : public MDNode
  {
    int headerSize;
    MDHeaderNode(int size, std::string text) : MDNode(text), headerSize(size) { };

    std::string toString() override
    {
      std::string tagOpen, tagClose;
      switch(headerSize)
      {
        case 1: tagOpen = "<h1>"; tagClose = "</h1>"; break;
        case 2: tagOpen = "<h2>"; tagClose = "</h2>"; break;
        case 3: tagOpen = "<h3>"; tagClose = "</h3>"; break;
        case 4: tagOpen = "<h4>"; tagClose = "</h4>"; break;
        case 5: tagOpen = "<h5>"; tagClose = "</h5>"; break;
        case 6: tagOpen = "<h6>"; tagClose = "</h6>"; break;
        default: tagOpen = tagClose = "";
      }
      return tagOpen + text + tagClose;
    }
  };

  struct MDParagraphNode : public MDContainerNode
  {
    MDParagraphNode() { }
    MDParagraphNode(std::string text) : MDContainerNode(text) { };

    std::string toString() override
    {
      return "<p>" + text + "</p>";
    }
  };

  struct MDListNode : public MDContainerNode
  {
    int startItemNumber;
    bool ordered;

    MDListNode() 
      : MDContainerNode(), startItemNumber(0), ordered(false) { };

    MDListNode(int startItemNumber) 
      : MDContainerNode(), startItemNumber(startItemNumber), ordered(true) { };

    std::string toString() override
    {
      std::string content;
      for (MDNode& node : children)
      {
        content += "<li>" + node.toString() + "</li>";
      }

      if (ordered)
        content = "<ol>" + content + "</ol>";
      else
        content = "<ul>" + content + "</ul>";
      return content;
    }
  };

  MDToken getMDToken(ParseContext& context)
  {
    MDToken token;
    token.type = MDToken::TOKEN_H1;
    token.start = (char*) context.source;
    token.end = (char*) context.eof;

    if (iseof(context))
    {
      token.type = token::type::token_eof;
      return token;
    }
  }

  std::string markdownToHtml(char* start, char* end)
  {
    ParseContext context{nullptr, start, end, start};

    MDToken token = getMDToken(context);
      std::vector<MDNode> document;
    return std::string(start, end - start);
  }


}
