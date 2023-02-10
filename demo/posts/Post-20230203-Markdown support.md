{{"Markdown support!"}}

Posts are now written in *Markdown* instead of plain *HTML* and I'll be slowly improving it's *Markdown* support

## What's supported so far

### Lists

* Item 1
   * sub item 1
* Item 2
* Item 3

### Paragraphs

To create paragraphs, use a blank line to separate one or more lines of text.
A paragraph can have multiple lines.

Also, a paragraph can be defined as a long sequence of sentences without line breaks. This sentense is on the same line as the previous one and no line breaks will be added the text includes it.

### Headings

Headings in *Markdown* are defined with 1 to 6 '#' consecutive characters

      # H1 Heading
      ## H2 Heading
      ### H3 Heading
      #### H4 Heading
      ##### H5 Heading
      ###### H6 Heading

### Emphasis
Supported emphasis are **BOLD**, *ITALIC* and ~~STRIKETHROUGH~~
To bold text, add two asterisks or underscores before and after a word or phrase

      **This text is bold** 
      __This text is bold__

To italicize text, add one asterisk or underscore before and after a word or phrase. 

      *This is a valid italic text!*
      _This is also a valid italic text!_

To get strikethrough text, add two tilde before and after a word or phrase
undesrscore character

      ~~This is a valid strikethrough text!~~


### Blocks of code

Any text preceeded by 6 spaces will be considered code and will be displayed as is.

      // Your First C++ Program
      #include <iostream>
      int main() {
          std::cout << "Hello Sailor!";
          return 0;
      }


### Blockqoutes

To add blockquotes simple prefix the text with '>' character

> Generalizations are always wrong

It's possible to nest blockquotes

> And now, for something completely different:
>> Generalizations are always wrong



### Links

Links in *Markdown* are defined with \[link text\]\(url\)
This is a [link](index.html) to the github page of this tool.

It's possible to nest emphasis on links

This is a [a **bold** and *itaclic* ~~striked~~ link](index.html) 


### Images

Images in *Markdown* are defined with !\[Image alt text\]\(url\)
![Image from https://www.posterlounge.co.uk/p/716218.html](assets/images/cleese.jpg)


### More on the go

Ordered and unordered lists are on the way as well as nesting block level elements like (headings, paragraphs, code blocks and blockquotes) inside list items.

Have fun!
