//TODO(marcio): Provide some way to sort collections
//TODO(marcio): Should I implement IF/ELSE commands ?
//FIXME(marcio): Iterating a list of 0 posts should work as expected but it's causing a syntax error instead

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif
#include <filesystem>
#include <stdio.h>
#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include <cctype>

#define logError(fmt, ...) printf("ERROR: "##fmt"", __VA_ARGS__)
#define logInfoFmt(fmt, ...) printf("INFO: "##fmt"", __VA_ARGS__)
#define logInfo(msg) printf("INFO: "##msg"")
#define END_OF_FILE -1

// site structure
struct Page
{
  std::string title;
  std::string relativeUrl;
  std::string sourceFileName;
  std::string outputFileName;
  Page(std::string& title,
      std::string& relativeUrl,
      std::string& sourceFileName,
      std::string& outFileName):
    title(title),
    relativeUrl(relativeUrl),
    sourceFileName(sourceFileName),
    outputFileName(outFileName) {}
};

struct Post : public Page
{
  std::string layoutName;
  std::string year;
  std::string month;
  std::string day;
  std::string monthName;

  Post(std::string title,
      std::string& relativeUrl,
      std::string& sourceFileName,
      std::string& outFileName,
      std::string& layoutName,
      std::string& day,
      std::string& month,
      std::string& year,
      std::string& monthName):
    Page(title, relativeUrl, sourceFileName, outFileName),
    layoutName(layoutName),
    day(day),
    month(month),
    year(year),
    monthName(monthName)
    {}

  
};

// parsing
struct Token
{
  enum Type
  {
    TOKEN_ASSIGN,           // =
    TOKEN_EOL,              // \n
    TOKEN_EXPRESSION_START, // {{
    TOKEN_EXPRESSION_END,   // }}
    TOKEN_INCLUDE,          // include
    TOKEN_FOR,              // for
    TOKEN_ENDFOR,           // endfor
    TOKEN_IN,               // in
    TOKEN_IDENTIFIER,       // similar to C variable name restrictions
    TOKEN_COLLECTION_PAGE,  // all_pages
    TOKEN_COLLECTION_POST,  // all_posts
    TOKEN_PATH,             // Path between double quotes "foo/bar"
    TOKEN_UNKNOWN           = -1,     // Any unknown token 
    TOKEN_EOF               = -2,     // EOF
  };

  Type type;
  char* start;
  char* end;
};

struct ParseContext
{
  const char* fileName;
  const char* source;
  char* eof;
  char* p;
};

char* readFileToBuffer(const char* fileName, size_t* fileSize = nullptr)
{
  std::ifstream is(fileName, std::ifstream::binary);
  if(!is)
  {
    logError("Could not open file '%s' for reading\n", fileName);
    return nullptr;
  }

  is.seekg (0, is.end);
  size_t length = is.tellg();
  is.seekg (0, is.beg);

  if(fileSize)
    *fileSize = length;

  char* buffer = new char[length];
  is.read (buffer,length);
  is.close();
  return buffer;
}

bool substrCompare(char* str, char* start, char* end)
{
  const int len = (int)(end - start);
  for(int i=0; i < len; i++)
  {
    if (*str == 0 || *str != tolower(*start))
    {
      return false;
    }
    ++start;
    ++str;
  }
  return true;
}

inline bool isEof(ParseContext& context) 
{
  return context.p >= context.eof; 
}

inline bool isWhiteSpace(char c) 
{
  return c == ' ' || c == '\t'; 
}

char getc(ParseContext& context)
{
  if(isEof(context))
    return END_OF_FILE;

  return *(context.p++);
}

char peek(ParseContext& context)
{
  if(isEof(context))
    return END_OF_FILE;

  return *(context.p);
}

bool isDigit(char c) 
{
  return (c >= '0' && c <= '9');
}

bool isLetter(char c) 
{
  return (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z'); 
}

void skipWhiteSpace(ParseContext& context)
{
  while(isWhiteSpace(*context.p))
  {
    getc(context);
  }
}

Token getToken(ParseContext& context)
{
  skipWhiteSpace(context);
  Token token;

  if (isEof(context))
  {
    token.type = Token::Type::TOKEN_EOF;
    return token;
  }

  token.type = Token::Type::TOKEN_UNKNOWN;
  token.start = context.p;
  token.end = token.start;
  char c = getc(context);
  char nextc = peek(context);

  // TOKEN_EXPRESSOIN_START
  if (c == '{' && nextc == '{')
  {
    getc(context);
    token.type = Token::Type::TOKEN_EXPRESSION_START;
    token.end+=2;
    return token;
  }

  // TOKEN_EXPRESSOIN_END
  if (c == '}' && nextc == '}')
  {
    getc(context);
    token.type = Token::Type::TOKEN_EXPRESSION_END;
    token.end+=2;
    return token;
  }
  
  // TOKEN_ASSIGN
  if (c == '=')
  {
    getc(context);
    token.type = Token::Type::TOKEN_ASSIGN;
    token.end++;
    return token;
  }

  // TOKEN_EOL \n
  if (c == '\n')
  {
    getc(context);
    token.type = Token::Type::TOKEN_EOL;
    token.end++;
    return token;
  }

  if (c == '\r' && nextc == '\n')
  {
    getc(context);
    token.type = Token::Type::TOKEN_EOL;
    token.end+=2;
    return token;
  }

  // TOKEN_PATH
  if (c == '\"')
  {
    ++token.start; // skip starting double quotes
    do{
      if (isEof(context))
      {
        // unexpected EOF while while parsing PATH token;
        token.type = Token::Type::TOKEN_UNKNOWN; 
        return token;
      }
      c = getc(context);
      ++token.end;
    } while (c != '\"');

    token.type = Token::Type::TOKEN_PATH;
    return token;
  }

  if (isLetter(c) || c == '_')
  {
    while(isLetter(c) || isDigit(c) || c == '_' || c == '-' || c == '.')
    {
      ++token.end;
      c = getc(context);
    }
    context.p--;

    if (substrCompare((char*)"for", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_FOR;
    }
    else if (substrCompare((char*)"endfor", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_ENDFOR;
    }
    else if (substrCompare((char*)"in", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_IN;
    }
    else if (substrCompare((char*)"include", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_INCLUDE;
    }
    else if (substrCompare((char*)"all_pages", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_COLLECTION_PAGE;
    }
    else if (substrCompare((char*)"all_posts", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_COLLECTION_POST;
    }
    else
    {
      token.type = Token::Type::TOKEN_IDENTIFIER;
    }
  }

  return token;
}

std::set<std::filesystem::path>* scanDirectory(std::filesystem::path& path, const char* extension)
{
  auto* fileList = new std::set<std::filesystem::path>();

  for(auto& p : std::filesystem::directory_iterator(path))
  {
    std::filesystem::path& subpath = (std::filesystem::path&) p.path();
    if (subpath.extension().compare(extension) == 0)
    {
      fileList->emplace(subpath);
    }
  }

  if(!fileList->size())
  {
    fileList = nullptr;
    delete fileList;
  }

  return fileList;
}

std::string& toLower(std::string& str)
{
  size_t len = str.length();
  for(int i=0; i < len; i++)
  {
    char c = str[i];

    str[i] = tolower(c);
  }
  return str;
}

bool requireToken(ParseContext& context, Token::Type requiredType, Token* tokenFound = nullptr)
{
  Token token = getToken(context);
  if(tokenFound) *tokenFound = token;
  if (token.type != requiredType)
  {
    logError("Unexpected token type '%d' while expecting '%d'\n", token.type, requiredType);
    return false;
  }
  return true;
}

std::unordered_map<std::string, std::string>* loadSiteConfigFile(std::filesystem::path& siteConfigFile)
{
  size_t bufferSize;
  std::string& fileName = siteConfigFile.string();
  char* buffer = readFileToBuffer(fileName.c_str(), &bufferSize);
  if(! buffer)
  {
    logError("Unable to open site config file '%s'\n", fileName.c_str());
    return nullptr;
  }

  ParseContext context;
  context.fileName = fileName.c_str();
  context.source = buffer;
  context.eof = buffer + bufferSize;
  context.p = (char*) context.source;

  bool success = true;
  std::unordered_map<std::string, std::string> *variablesPtr = new std::unordered_map<std::string, std::string>();
  std::unordered_map<std::string, std::string>& variables = *variablesPtr;

  // set some default values
  variables["site.name"] = "Undefined";
  variables["site.url"] = "http://";
  variables["site.template_dir"] = "template";
  variables["site.posts_src_dir"] = "posts";
  variables["month_01"] = "JAN";
  variables["month_02"] = "FEB";
  variables["month_03"] = "MAR";
  variables["month_04"] = "APR";
  variables["month_05"] = "MAR";
  variables["month_06"] = "JUN";
  variables["month_07"] = "JUL";
  variables["month_08"] = "AGO";
  variables["month_09"] = "SEP";
  variables["month_10"] = "AUG";
  variables["month_11"] = "OCT";
  variables["month_12"] = "DEC";

  while (context.p < context.eof)
  {
    Token value;
    Token key = getToken(context);

    if (key.type == Token::Type::TOKEN_EOL)
    {
      continue;
    }

    if (! (requireToken(context, Token::Type::TOKEN_ASSIGN)
        && requireToken(context, Token::Type::TOKEN_PATH, &value)))
    {
      success = false;
      break;
    }

    //this might be an EOL line or an EOF
    Token token = getToken(context);
    if (token.type != Token::Type::TOKEN_EOL && token.type != Token::Type::TOKEN_EOF)
    {
      success = false;
      break;
    }

    std::string sKey = std::string(key.start, key.end - key.start);
    std::string sValue = std::string(value.start, value.end - value.start);
    variables[sKey] = sValue;
  }

  // if templates and posts path is not absolute, make it relative to site.txt file
  std::filesystem::path templatePath = siteConfigFile;
  templatePath.remove_filename();

  if (templatePath.is_relative())
  {
    templatePath.append(variables["site.template_dir"]);
    variables["site.template_dir"] = templatePath.string();
  }

  std::filesystem::path postsPath = siteConfigFile;
  postsPath.remove_filename();
  if (postsPath.is_relative())
  {
    postsPath.append(variables["site.posts_src_dir"]);
    variables["site.posts_src_dir"] = postsPath.string();
  }

  delete buffer;

  if (!success)
  {
    logError("Error parsing site config file.\n");
    delete variablesPtr;
    return nullptr;
  }

  return variablesPtr;
}

size_t processSource(
    std::ofstream& outStream,
    std::filesystem::path& templateRoot,
    std::unordered_map<std::string, std::string>& variables,
    std::vector<Page>& pageList,
    std::vector<Post>& postList,
    const char* sourceStart,
    const char* sourceEnd);

bool parseExpression(ParseContext& context,
    std::ofstream& outStream,
    std::filesystem::path& templateRoot,
    std::unordered_map<std::string, std::string>& variables,
    std::vector<Page>& pageList,
    std::vector<Post>& postList)
{
  // expressions MUST start with TOKEN_EXPRESSION_START
  Token token;
  if (!requireToken(context, Token::Type::TOKEN_EXPRESSION_START, &token))
  {
    return false;
  }

  // save the address of the expression so we can return to it in case of a end of block
  const char* expressionStart = token.start;
  token = getToken(context);

  switch(token.type)
  {
    // VARIABLE
    case Token::Type::TOKEN_IDENTIFIER:
      {
        size_t identifierLen = token.end - token.start;
        auto it = variables.find(std::string(token.start, identifierLen));

        std::string variableValue = "UNDEFINED";
        if (it == variables.end())
        {
          logError("Unknown variable '%.*s'\n", (int)identifierLen, token.start);
        }
        else
        {
          variableValue = (*it).second;
        }
        outStream.write(variableValue.c_str(), variableValue.length());
        return requireToken(context, Token::Type::TOKEN_EXPRESSION_END, &token);
      }
      break;

      // INCLUDE
    case Token::Type::TOKEN_INCLUDE:
      {
        if (!requireToken(context, Token::Type::TOKEN_PATH, &token))
          return false;
        if (!requireToken(context, Token::Type::TOKEN_EXPRESSION_END))
          return false;

        // include path is always relative to template root
        std::string includedPagePath = (templateRoot / std::string(token.start, token.end - token.start)).string();

        size_t includeFileSize;
        char* includedSourceStart = readFileToBuffer(includedPagePath.c_str(), &includeFileSize);
        char* includedSourceEnd = includedSourceStart + includeFileSize;
        bool includeSuccess = processSource(outStream, templateRoot, variables, pageList, postList, includedSourceStart, includedSourceEnd);
        delete includedSourceStart;
        return includeSuccess;
      }
      break;


      // FOREACH
    case Token::Type::TOKEN_FOR:
      {
        if (!requireToken(context, Token::Type::TOKEN_IDENTIFIER, &token))
          return false;

        const char* expressionStart = token.start;

        std::string iteratorName = std::string(token.start,  token.end - token.start);

        if (!requireToken(context, Token::Type::TOKEN_IN, &token))
          return false;

        token = getToken(context);
        Token::Type collectionType = token.type;
        size_t numIterations = 0;

        if (collectionType == Token::Type::TOKEN_COLLECTION_PAGE)
        {
          numIterations = pageList.size();
        }
        else if(collectionType == Token::Type::TOKEN_COLLECTION_POST)
        {
          numIterations = postList.size();
        }
        else
          return false;

        if (!requireToken(context, Token::Type::TOKEN_EXPRESSION_END, &token))
        {
          return false;
        }

        char* blockSourceStart = token.end;
        size_t advance = 0;
        for(int i=0; i < numIterations; i++)
        {
          if (collectionType == Token::Type::TOKEN_COLLECTION_PAGE)
          {
            Page& page = pageList[i];
            variables[iteratorName + ".title"] = page.title;
            variables[iteratorName + ".url"] = page.relativeUrl;
          }
          else if (collectionType == Token::Type::TOKEN_COLLECTION_POST)
          {
            Post& post = postList[i];
            variables[iteratorName + ".title"] = post.title;
            variables[iteratorName + ".url"] = post.relativeUrl;
            variables[iteratorName + ".layout"] = post.layoutName;
            variables[iteratorName + ".year"] = post.year;
            variables[iteratorName + ".month"] = post.month;
            variables[iteratorName + ".day"] =  post.day;
            variables[iteratorName + ".month_name"] = post.monthName;
          }
          else
          {
            //This should NEVER happen
            logError("Uknown collection type.\n");
          }

          variables[iteratorName + ".number"] = std::to_string(i);
          advance = processSource(outStream, templateRoot, variables, pageList, postList, blockSourceStart, context.eof);

          if (advance < 0)
          {
            logError("Error parsing foreach block\n");
            return false;
          }
        }

        // remove variables for this iteration
        variables.erase(iteratorName + ".title");
        variables.erase(iteratorName + ".url");
        variables.erase(iteratorName + ".layout");
        variables.erase(iteratorName + ".number");
        context.p = blockSourceStart + advance;

        return requireToken(context, Token::Type::TOKEN_EXPRESSION_START) &&
          requireToken(context, Token::Type::TOKEN_ENDFOR) &&
          requireToken(context, Token::Type::TOKEN_EXPRESSION_END);

      }
      break;

      // FOREACH-END
    case Token::Type::TOKEN_ENDFOR:
      {
        //end of blocks, are not parsed along with expressions, but with blocks
        context.p = (char*) expressionStart;
        return true;
      }
      break;

    default:
      return false;
  }
}

// Returns how much of the source was parsed, or NEGATIVE value in case of an error
size_t processSource(
    std::ofstream& outStream,
    std::filesystem::path& templateRoot,
    std::unordered_map<std::string, std::string>& variables,
    std::vector<Page>& pageList,
    std::vector<Post>& postList,
    const char* sourceStart,
    const char* sourceEnd)
{
  char* p = (char*)sourceStart;
  char* writeStart = p;
  size_t writeSize = 0;

  while(p < sourceEnd)
  {
    //found an expression
    if (*p == '{'  && (p + 1) < sourceEnd && *(p+1) == '{')
    {
      if (writeSize > 0)
      {
        outStream.write(writeStart, writeSize);
      }

      ParseContext context;
      context.source = p;
      //context.fileName = sourceFileName.c_str();
      context.eof = (char*) sourceEnd;
      context.p = (char*) context.source;

      if (!parseExpression(context, outStream, templateRoot, variables, pageList, postList))
      {
        return -1;
      }
      writeSize = 0;
      // Nothing was parsed. Probably an end of block. So we exit now.
      if (p == context.p)
        break;

      p = context.p;// we continue from where the last expression ended
      writeStart = p; 
    }
    else
    {
      ++writeSize;
      ++p;
    }
  }

  if (writeSize > 0)
  {
    outStream.write(writeStart, writeSize);
  }

  // We return how further we went down the source code. We migh have reached
  // the end or just a block end like {{endfor}}
  size_t advance = p - sourceStart;
  return advance;
}

bool processPage(
    std::filesystem::path& templateRoot,
    std::string& sourceFileName, 
    std::string& outputFileName, 
    std::vector<Page>& pageList, 
    std::vector<Post>& postList,
    std::unordered_map<std::string, std::string>& variables)
{
  std::ofstream outStream(outputFileName);
  if(!outStream.is_open())
  {
    logError("Could not write to file %s\n", outputFileName.c_str());
    return false;
  }

  size_t fileSize;
  char* sourceStart = readFileToBuffer(sourceFileName.c_str(), &fileSize);
  char* sourceEnd = sourceStart + fileSize;
  if(!sourceStart)
  {
    logError("Unable to read from template '%s'", sourceFileName.c_str());
    outStream.close();
    return false;
  }

  bool result = processSource(outStream, templateRoot, variables, pageList, postList, sourceStart, sourceEnd);

  if (!result)
  {
    logError("Failed to process '%s'\n", sourceFileName.c_str());
  }

  delete[] sourceStart;
  outStream.close();
  return result;
}

int generateSite(std::filesystem::path& inputDirectory, std::filesystem::path& outputDirectory)
{

  std::vector<Page> pageList;
  std::vector<Post> postList;
  bool hasErrors = false;
  bool hasWarnings = false;

  // Try to create the output directory in case it does not exists
  std::filesystem::create_directories(outputDirectory);

  std::filesystem::path siteConfigFile = inputDirectory / "site.txt";
  std::unordered_map<std::string, std::string>* variablesPtr = loadSiteConfigFile(siteConfigFile);
  std::unordered_map<std::string, std::string>& variables = *variablesPtr;
  std::filesystem::path templateDirectory = variables["site.template_dir"]; 
  std::filesystem::path postsDirectory = variables["site.posts_src_dir"]; 
  std::filesystem::path layoutDirectory = templateDirectory / "layout";

  // Collect Page info
  std::set<std::filesystem::path>* pageFiles = scanDirectory(templateDirectory, ".html");
  if (pageFiles)
  {
    for(const std::filesystem::path& path : *pageFiles)
    {
      std::string fileName = path.filename().string();
      std::string title = fileName.substr(0, fileName.find("."));
      std::string relativeUrl = toLower((std::string&)fileName);
      std::string sourceFileName = (templateDirectory / fileName).string();
      std::string outputFileName = (outputDirectory / relativeUrl).string();

      pageList.emplace_back(title, relativeUrl, sourceFileName, outputFileName);
    }
    delete pageFiles;
  }

  // Add some extra dynamic variables
  variables["site.num_pages"] = std::to_string((int)pageList.size());
  variables["site.num_posts"] = std::to_string((int)postList.size());

  // Collect Content and Layout info
  std::set<std::filesystem::path>* postFiles = scanDirectory(postsDirectory, ".txt");
  std::set<std::filesystem::path>* layoutFiles = scanDirectory(layoutDirectory, ".html");
  postFiles->erase(siteConfigFile); // ignore the site config file

  if (postFiles)
  {
    for(auto& it = postFiles->rbegin(); it != postFiles->rend(); ++it)
    {
      std::string& fileName = (*it).filename().string();
      std::string layoutName = fileName.substr(0, fileName.find("-"));
      std::string timestamp  = fileName.substr(layoutName.length() + 1, 8); //AAAAMMDD = 8 chars
      const size_t layoutNameLen = layoutName.length();
      const size_t titleLen = fileName.length() - layoutNameLen - 10 - 4; // -AAAAMMDD- = 10 chars; .txt = 4 chars

      // Fill in the content data
      std::string title = fileName.substr(layoutName.length() + 10, titleLen);
      std::string sourceFileName = (postsDirectory / fileName).string();
      std::string relativeUrl = timestamp + "_" + title + ".html";
      toLower(relativeUrl);
      std::string day = timestamp.substr(6, 2).c_str();
      std::string month = timestamp.substr(4, 2).c_str();
      std::string year = timestamp.substr(0, 4).c_str();
      std::string monthName = variables["month_" + month];
      std::string outputFileName = (outputDirectory / relativeUrl).string();

      int dayValue = std::atoi(day.c_str());
      int monthValue = std::atoi(month.c_str());
      int yearValue = std::atoi(month.c_str());

      // Does it have a valid timestamp ?
      if (dayValue == 0 || monthValue == 0 || yearValue == 0 || dayValue > 30 || monthValue > 12)
      {
        hasWarnings = true;
        logError("Invalid date format on content file '%s'.\n", fileName.c_str());
      }

      // Does it have a valid layout ?
      std::string layoutFileName = (layoutDirectory / layoutName).concat(".html").string();
      toLower(layoutFileName);
      if (layoutFiles->find(layoutFileName) == layoutFiles->end())
      {
        hasWarnings = true;
        logError("Invalid Layout file referenced on content file '%s'.\n", fileName.c_str());
      }

      if (hasWarnings)
      {
        logError("Skipping file '%s'.\n", fileName.c_str());
        continue;
      }

      postList.emplace_back(title, relativeUrl, sourceFileName, outputFileName,
          layoutName, day, month, year, monthName);
    }

    delete layoutFiles;
    delete postFiles;
  }

  for(Page& page : pageList)
  {
    logInfoFmt("Processing page %s\n", page.sourceFileName.c_str());
    variables["page.title"] = page.title;
    variables["page.url"] = page.relativeUrl;
    processPage(templateDirectory, page.sourceFileName, page.outputFileName, pageList, postList, variables);
  }

  for(Post& post : postList)
  {
    logInfoFmt("Processing post %s\n", post.sourceFileName.c_str());

    // load content file
    size_t contentSourceSize;
    char* contentSource = readFileToBuffer(post.sourceFileName.c_str(), &contentSourceSize);

    std::string layoutFileName = (layoutDirectory / post.layoutName).concat(".html").string();
    std::string outputFileName = (outputDirectory / post.relativeUrl).string();

    if (contentSource == nullptr)
    {
      hasErrors = true;
      break;
    }

    // Export each post data as a "post.xxx" variable
    variables["post.title"] = post.title;
    variables["post.layout"] = post.layoutName;
    variables["post.url"] = post.relativeUrl;
    variables["post.body"] = std::string(contentSource, contentSourceSize);
    variables["post.year"] = post.year;
    variables["post.month"] = post.month;
    variables["post.day"] = post.day;
    variables["post.month_name"] = post.monthName;
    // Consider the template data as the page data
    variables["page.title"] = post.title;
    variables["page.url"] = post.relativeUrl;
    

    // we need a "fake" page to pass to processPage
    //Page page(content.title, content.relativeUrl, layoutFileName);
    bool success = processPage(
        templateDirectory,
        layoutFileName,
        outputFileName,
        pageList,
        postList,
        variables);
    delete[] contentSource;

    if (! success)
      hasErrors = true;
  }

  if (hasErrors)
    return -1;

  if (hasWarnings)
    return 1;


  logInfo("Copying assets ...\n");
    std::filesystem::copy(templateDirectory / "assets", outputDirectory / "assets",
        std::filesystem::copy_options::recursive |
        std::filesystem::copy_options::overwrite_existing 
        );
  logInfo("Done\n");
  return 0;
}

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    printf("%s <path_to_site_folder> <output_directory>\n", argv[0]);
    return 0;
  }

  return generateSite(std::filesystem::path(argv[1]),
      std::filesystem::path(argv[2]));
}

