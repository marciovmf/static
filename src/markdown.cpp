//TODO(marcio): It's not outputting underscore character on some posts
//TODO(marcio): Implement lists

#include <iostream>
#include <fstream>
#include <string>
#include <regex>

using namespace std;

string getSpanLevelFormatting(const string& line);

string replaceScapeSequences(string line)
{
  size_t pos = line.find("\\");
  while (pos != string::npos)
  {
    if(line[pos+1] == '_')
      line.replace(pos, 2, "&#95;");
    else if(line[pos+1] == '[')
      line.replace(pos, 2, "&#91;");
    else if(line[pos+1] == ']')
      line.replace(pos, 2, "&#93;");
    else if(line[pos+1] == '<')
      line.replace(pos, 2, "&lt;");
    else if(line[pos+1] == '>')
      line.replace(pos, 2, "&gt;");
    else if(line[pos+1] == '(')
      line.replace(pos, 2, "&#10098;");
    else if(line[pos+1] == ')')
      line.replace(pos, 2, "&#10099;");
    else if(line[pos+1] == '\\')
      line.replace(pos, 2, "&#92;");

    pos = line.find("\\", pos+1);
  }

  return line;
}

string getHeader(string line) 
{
  int count = 0;
  while (line[count] == '#')
    count++;

  if (count > 5) return getSpanLevelFormatting(line);

  string countString = to_string(count);
  return "<h" + countString + ">" + getSpanLevelFormatting(line.substr(count + 1)) + "</h" + countString + ">";
}

string getListItem(string line) 
{
  return "<li>" + getSpanLevelFormatting(line.substr(3)) + "</li>";
}

string getLink(const string&& line) 
{
  smatch match;

  if(line.find("[") != string::npos)
  {
    static regex pattern("(.*)\\[(.*)\\]\\((.*)\\)(.*)");
    if (regex_search(line, match, pattern)) 
    {
      return getLink(match[1].str()) + "<a href=\"" + match[3].str() + "\">" + match[2].str() + "</a>" + getLink(match[4].str());
    }
  }
  return line;
}

string getImage(const string& line) 
{
  smatch match;

  if(line.find("![") != string::npos)
  {
    static regex pattern("(.*)!\\[(.*)\\]\\((.*)\\)(.*)");
    if (regex_search(line, match, pattern)) 
    {
      return match[1].str() + "<img src=\"" + match[3].str() + "\" alt=\"" + match[2].str() + "\">" + match[4].str();
    }
  }
  return line;
}

string getEmphasis(const string&& line) 
{
  if (line.find_first_of("*_~") != string::npos)
  {
    static regex strongPattern("(\\*\\*)(.*?)\\1");
    string result = regex_replace(line, strongPattern, "<strong>$2</strong>");
    static regex emPattern("(\\_|\\*)(.*?)\\1");
    result = regex_replace(result, emPattern, "<em>$2</em>");
    static regex strikPattern("(\\~\\~)(.*?)\\1");
    result = regex_replace(result, strikPattern, "<s>$2</s>");
  }

  return line;
}

string getSpanLevelFormatting(const string& line)
{
  return getEmphasis(replaceScapeSequences(getLink(getImage(line))));
}

string markdownToHtml(string sourceFile)
{
  string line;
  string html;
  ifstream markdownFile(sourceFile);

  if (markdownFile.is_open()) 
  {
    while (getline(markdownFile, line)) 
    {
      if (line.empty()) continue;

      if (line[0] == '#') { html += getHeader(line);
      }
      else if (line[0] == '*') 
      {
        html += "<ul>\n" + getListItem(line) + "</ul>";
      }
      else if (line[0] == '1' && line[1] == '.') 
      {
        html += "<ol>\n" + getListItem(line) + "</ol>";
      }
      else if(line.rfind("      ", 0) == 0) // 6 spaces means code block
      {
        string paragraph;
        do
        {
          // It's only required to indent the first line. But if next lines are
          // indented, we must trim the unnecessary spaces
          if(line.rfind("      ", 0) == 0)
            line.erase(0, 6);

          paragraph += line + "\n";
          getline(markdownFile, line);
        }
        while(!line.empty());
        html += "<pre><code>" + paragraph + "</code></pre>";
      }
      else if(line[0] == '>')
      {
        string paragraph;
        int depth = 0;
        int indentCharIndex = 0;
        int lineCount = 0;
        do
        {
          int newDepth = 0;
          for(int i = 0; i < line.length(); i++) 
          {
            if (line[i] == '>')
            {
              indentCharIndex = i + 1;
              newDepth++;
            }

            if (line[i] == ' ')
              continue;
          }

          if (newDepth > depth)
          {
            depth = newDepth;
            paragraph += "<blockquote><p>";
            lineCount = 0;
          }

          line.erase(0, indentCharIndex);
          paragraph += (lineCount++ > 0) ? "<br>" + line : line;
          getline(markdownFile, line);
        }
        while(!line.empty());


        while(depth)
        {
          depth--;
          paragraph += "</p></blockquote>";
        }

        html += paragraph;
      }
      else
      {
        string paragraph;
        int lineCount = 0;
        do
        {
          paragraph += 
            lineCount++ ?
            "<br>" + getSpanLevelFormatting(line):
            getSpanLevelFormatting(line);

          getline(markdownFile, line);
        }
        while(!line.empty());
        html += "<p>" + paragraph + "</p>";
      }
    }
  }

  return html;
}

