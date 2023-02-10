//TODO(marcio): Should I implement IF/ELSE commands ?
//TODO(marcio): Make possible to iterate a subsection of a collection

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
#include <algorithm>
#include <any>
#include <chrono>
#include "parser_utils.h"
#include "markdown.h"

template<typename T>
using CompareFunction = bool(*)(const T&, const T&);

template<typename T>
struct SortingInformation
{
  bool ascending = true;
  CompareFunction<T> compareFunction = nullptr;
  SortingInformation<T>(CompareFunction<T> f):compareFunction(f) {}
};


// site structure
struct Page
{
  static SortingInformation<Page> sorting;
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

  static bool compareByTitle(const Page& a, const Page& b)
  {
    return Page::sorting.ascending ? a.title < b.title : a.title > b.title;
  }

  static bool compareByUrl(const Page& a, const Page& b)
  {
    return Page::sorting.ascending ? a.relativeUrl < b.relativeUrl : a.relativeUrl > b.relativeUrl;
  }

  static void compareBy(const std::string& member, bool ascending = true)
  {
    Page::sorting.ascending = ascending;
    if (member == "title")
      Page::sorting.compareFunction = (CompareFunction<Page>) Page::compareByTitle;
    else if (member == "url")
      Page::sorting.compareFunction = (CompareFunction<Page>) Page::compareByUrl;
    else if (member == "title")
      Page::sorting.compareFunction = (CompareFunction<Page>) Page::compareByTitle;
    else
    {
      logError("Unable to sort Page list by unknown property '%s'", member.c_str());
      Page::sorting.compareFunction = (CompareFunction<Page>) Page::compareByTitle;
    }
  }
};

struct Post : public Page
{
  static SortingInformation<Post> sorting;
  std::string layoutName;
  std::string year;
  std::string month;
  std::string day;
  std::string monthName;
  int yearInt;
  int monthInt;
  int dayInt;

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
    year(year),
    month(month),
    day(day),
    monthName(monthName),
    yearInt(std::atoi(year.c_str())),
    monthInt(std::atoi(month.c_str())),
    dayInt(std::atoi(day.c_str()))
  {
  }

  bool isAttribute(std::string& attributeName) 
  {
    return attributeName == "title" 
      || attributeName == "relativeUrl"
      || attributeName == "title"
      || attributeName == "url"
      || attributeName == "layout"
      || attributeName == "year"
      || attributeName == "month"
      || attributeName == "day"
      || attributeName == "month_name";
  }

  static bool compareByTitle(const Post& a, const Post& b)
  {
    return Post::sorting.ascending ? a.title < b.title : a.title > b.title;
  }

  static bool compareByUrl(const Post& a, const Post& b)
  {
    return Post::sorting.ascending ? a.relativeUrl < b.relativeUrl : a.relativeUrl > b.relativeUrl;
  }

  static bool compareByLayout(const Post& a, const Post& b)
  {
    return Post::sorting.ascending ? a.layoutName < b.layoutName : a.layoutName > b.layoutName;
  }

  static bool compareByDate(const Post& a, const Post& b)
  {
    if (Post::sorting.ascending)
      return a.yearInt < b.yearInt 
        || (a.yearInt == b.yearInt && a.monthInt < b.monthInt )
        || (a.yearInt == b.yearInt && a.monthInt == b.monthInt && a.dayInt < b.dayInt);
    else
      return a.yearInt > b.yearInt 
        || (a.yearInt == b.yearInt && a.monthInt > b.monthInt )
        || (a.yearInt == b.yearInt && a.monthInt == b.monthInt && a.dayInt > b.dayInt);
  }

  static bool compareByMonth(const Post& a, const Post& b)
  {
    bool result = a.yearInt <= b.yearInt && a.monthInt < b.monthInt;
    return Post::sorting.ascending ? result : !result;
  }

  static bool compareByYear(const Post& a, const Post& b)
  {
    bool result = a.yearInt < b.yearInt;
    return Post::sorting.ascending ? result : !result;
  }

  static void compareBy(const std::string& member, bool ascending = true)
  {
    Post::sorting.ascending = ascending;
    if (member == "title")
      Post::sorting.compareFunction = Post::compareByTitle;
    else if (member == "url")
      Post::sorting.compareFunction = Post::compareByUrl;
    else if (member == "layout")
      Post::sorting.compareFunction = Post::compareByLayout;
    else if (member == "year")
      Post::sorting.compareFunction = Post::compareByYear;
    else if (member == "month")
      Post::sorting.compareFunction = Post::compareByMonth;
    else if (member == "day" || member == "date")
      Post::sorting.compareFunction = Post::compareByDate;
    else
    {
      logError("Unable to sort Post list by unknown property '%s'", member.c_str());
      Post::sorting.compareFunction = (CompareFunction<Post>) Post::compareByDate;
    }
  }
};

SortingInformation<Page> Page::sorting = SortingInformation<Page>(Page::compareByTitle);
SortingInformation<Post> Post::sorting = SortingInformation<Post>(Post::compareByDate);


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
  for(size_t i=0; i < len; i++)
  {
    char c = str[i];

    str[i] = (char) tolower(c);
  }
  return str;
}

void logMismatchedTokenType(Token::Type expected, Token::Type found)
{
  logError("Unexpected token type '%d' while expecting '%d'\n", found, expected);
}

bool requireToken(ParseContext& context, Token::Type requiredType, Token* tokenFound = nullptr)
{
  Token token = getToken(context);
  if(tokenFound) *tokenFound = token;
  if (token.type != requiredType)
  {
    logMismatchedTokenType(requiredType, token.type);
    return false;
  }
  return true;
}

std::unordered_map<std::string, std::string>* loadSiteConfigFile(std::filesystem::path& siteConfigFile)
{
  size_t bufferSize;
  std::string fileName = siteConfigFile.string();
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
  variables["month_05"] = "MAY";
  variables["month_06"] = "JUN";
  variables["month_07"] = "JUL";
  variables["month_08"] = "AUG";
  variables["month_09"] = "SEP";
  variables["month_10"] = "OCT";
  variables["month_11"] = "NOV";
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

  std::filesystem::path siteRootFolder = siteConfigFile;
  siteRootFolder.remove_filename();

  // if templates dir is not absolute, consider it's relative to site.txt folder location
  std::filesystem::path templatesDir = variables["site.template_dir"];
  if (templatesDir.is_relative())
  {
    templatesDir = siteRootFolder / templatesDir;
    variables["site.template_dir"] = templatesDir.string();
  }

  // if posts_src dir is not absolute, consider it's relative to site.txt folder location
  std::filesystem::path postsSrcDir = variables["site.posts_src_dir"];
  if (postsSrcDir.is_relative())
  {
    postsSrcDir = siteRootFolder / postsSrcDir;
    variables["site.posts_src_dir"] = postsSrcDir.string();
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

// Markdown parsing

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

        if (outStream)
        {
          outStream.write(variableValue.c_str(), variableValue.length());
        }
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

        std::string iteratorName = std::string(token.start,  token.end - token.start);

        if (!requireToken(context, Token::Type::TOKEN_IN, &token))
          return false;

        token = getToken(context);
        Token::Type collectionType = token.type;
        size_t numIterations = 0;

        //check for orderby_asc <field> or orderby_dec <field>
        token = getToken(context);
        Token orderByToken = Token();
        Token::Type orderDirection = Token::Type::TOKEN_UNKNOWN;

        bool shouldOrder = false;
        if (token.type == Token::Type::TOKEN_ORDERBY_ASC 
            || token.type == Token::Type::TOKEN_ORDERBY_DESC)
        {
          shouldOrder = true;
          orderDirection = token.type;

          if (!requireToken(context, Token::Type::TOKEN_IDENTIFIER, &orderByToken) 
              || !requireToken(context, Token::Type::TOKEN_EXPRESSION_END, &token))
          {
            return false;
          }
        }
        else if (token.type != Token::Type::TOKEN_EXPRESSION_END)
        {
          logMismatchedTokenType(Token::Type::TOKEN_EXPRESSION_END, token.type);
          return false;
        }

        if (collectionType == Token::Type::TOKEN_COLLECTION_PAGE)
        {
          numIterations = pageList.size();
          if(shouldOrder)
          {
            std::string memberName = std::string(orderByToken.start, orderByToken.end);
            bool ascending = orderDirection == Token::Type::TOKEN_ORDERBY_ASC;
            Page::compareBy(memberName, ascending);
            std::sort(pageList.begin(), pageList.end(), Page::sorting.compareFunction);
          }
        }
        else if(collectionType == Token::Type::TOKEN_COLLECTION_POST)
        {
          numIterations = postList.size();
          if (shouldOrder)
          {
            std::string memberName = std::string(orderByToken.start, orderByToken.end);
            bool ascending = orderDirection == Token::Type::TOKEN_ORDERBY_ASC;
            Post::compareBy(memberName, ascending);
            std::sort(postList.begin(), postList.end(), Post::sorting.compareFunction);
          }
        }
        else
          return false;

        char* blockSourceStart = token.end;
        size_t advance = 0;
        if (numIterations == 0)
        {
          // If we are iterating an empty list, we still need to parse the
          // contents of the block in order to find the matching {{endfor}}
          // and get the correct advance value.
          // Because of that I'm passing a dummy stream and defining default
          // values to all variables.

          variables[iteratorName + ".title" ] = "undefined";
          variables[iteratorName + ".url"   ] = "undefined";
          variables[iteratorName + ".layout"] = "undefined";
          variables[iteratorName + ".year"  ] = "undefined";
          variables[iteratorName + ".month" ] = "undefined";
          variables[iteratorName + ".day"   ] = "undefined";
          variables[iteratorName + ".date"   ] = "undefined";
          variables[iteratorName + ".month_name"] = "undefined";

          std::ofstream dummy;
          advance = processSource(dummy, templateRoot, variables, pageList, postList, blockSourceStart, context.eof);
        }

        for(size_t i=0; i < numIterations; i++)
        {
          if (collectionType == Token::Type::TOKEN_COLLECTION_PAGE)
          {
            Page& page = pageList[i];

            variables[iteratorName + ".title" ] = page.title;
            variables[iteratorName + ".url"   ] = page.relativeUrl;
          }
          else if (collectionType == Token::Type::TOKEN_COLLECTION_POST)
          {
            Post& post = postList[i];
            variables[iteratorName + ".title" ] = post.title;
            variables[iteratorName + ".url"   ] = post.relativeUrl;
            variables[iteratorName + ".layout"] = post.layoutName;
            variables[iteratorName + ".year"  ] = post.year;
            variables[iteratorName + ".month" ] = post.month;
            variables[iteratorName + ".day"   ] = post.day;
            variables[iteratorName + ".date"   ] = post.day;
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
    if (*p == '{'  && (p+1) < sourceEnd && *(p+1) == '{')
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
        return (size_t) -1;
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

int generateSite(std::filesystem::path&& inputDirectory, std::filesystem::path&& outputDirectory)
{
  auto start = std::chrono::system_clock::now();
  std::vector<Page> pageList;
  std::vector<Post> postList;
  bool hasErrors = false;
  bool hasWarnings = false;

  // Try to create the output directory in case it does not exists
  std::filesystem::remove_all(outputDirectory);
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
  std::set<std::filesystem::path>* postFiles = scanDirectory(postsDirectory, ".md");
  std::set<std::filesystem::path>* layoutFiles = scanDirectory(layoutDirectory, ".html");

  if (postFiles)
  {
    postFiles->erase(siteConfigFile); // ignore the site config file
    for(auto it = postFiles->rbegin(); it != postFiles->rend(); ++it)
    {
      std::string fileName = (*it).filename().string();
      std::string layoutName = fileName.substr(0, fileName.find("-"));
      std::string timestamp  = fileName.substr(layoutName.length() + 1, 8); //AAAAMMDD = 8 chars
      const size_t layoutNameLen = layoutName.length();
      const size_t titleLen = fileName.length() - layoutNameLen - 10 - 3; // -AAAAMMDD- = 10 chars; .md = 3 chars

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
        hasWarnings = false;
        continue;
      }


      // check for title override in the first line of the file
      std::ifstream postFile(sourceFileName);
      if (postFile.is_open())
      {
        std::string line;
        getline(postFile, line);
        ParseContext context;
        context.p = (char*) line.c_str();
        context.eof = (char*) (context.p + line.length());

        if ((getToken(context).type == Token::TOKEN_EXPRESSION_START))
        {
          Token tokenTitle;
          if (requireToken(context, Token::TOKEN_PATH, &tokenTitle)
              && requireToken(context, Token::TOKEN_EXPRESSION_END))
          {
            title = std::string(tokenTitle.start, tokenTitle.end - tokenTitle.start);
          }
          postFile.close();
      }
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

  //TODO(marcio): check if MD file exists
  std::string htmlSource = markdownToHtml(post.sourceFileName);

  // Export each post data as a "post.xxx" variable
  variables["post.title"] = post.title;
  variables["post.layout"] = post.layoutName;
  variables["post.url"] = post.relativeUrl;
  variables["post.body"] = htmlSource;
  variables["post.body"] = htmlSource;
  variables["post.year"] = post.year;
  variables["post.month"] = post.month;
  variables["post.day"] = post.day;
  variables["post.month_name"] = post.monthName;
  // Consider the template data as the page data
  variables["page.title"] = post.title;
  variables["page.url"] = post.relativeUrl;

  // we need a "fake" page to pass to processPage
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

  auto end = std::chrono::system_clock::now();
  auto markdownProcessTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  logInfoFmt("Site generated in %ldms\n", (long) markdownProcessTime);

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

  return generateSite(std::filesystem::path(argv[1]), std::filesystem::path(argv[2]));
}

