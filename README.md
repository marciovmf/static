# (Un)amazing

A minimalist monolithic static site generator.

## Why ?

I used to have a wordpress website. Wordpress is as powerfull as bloated and slow. It also breaks from time to time if you allow it to update automatically. So I decided "port" my personal blog as a static site. While looking for static site generators on the internet I realized almost all of them were also a bloated mess that required me to install a bunch of stuff just to replace some strings on a html files. Then I decided to spend a weekend write something that fits my needs.
This is a really simple program that builds as a single command line executable and requires no additional dependencies to build or run.

## Should I use it ?

No, but yes.
If you just need simple features this program gets you covered. If you need fancy frontend generation logic and don't mind installing a bunch of crap, you better not.

## How does it work ?

The program expects two arguments: the path where the site configuration file is and the path where the site will be generated to.
``` ussg <site_root> <output_root> ```

The _site_root_ must contain a site configuration file named **site.txt**

### The site.txt file

This file is used to set some defaults for the program and to point to some important directories. 
The file should have one ```key = "value"``` pair per line. Empty lines are ignored. 
The required keys are

- **site.name**           -> The name of the website. If not specified will default to "Undefined"
- **site.url**            -> The site url. If not specified will default to "http://"
- **site.template_dir**   -> Path to the directory where to look for template files, relative to _site_root_. If not specified will default to "template"
- **site.post_src_dir**   -> Path to the directory where to look for post files, relative to _site_root_. If not specified will default to "posts"
- **month_01** ~ **month_12** -> Month names. This can be used to output posting date with a custom month name. If not specified will deault to first 3 letters of english month names (JAN, FEB, MAR etc).

You can add extra keys here and use them on your own templates.

Here is an example of how the **site.txt** file should look like:

```
site.name = "Handmade Gamedev"
site.url = "http://handmadegame.dev/"
site.template_dir = "theme\default"
site.post_src_dir = "posts"
my_custom_key = "foo"
another_custom_key = "bar"

```


### Posts

Posts are special .txt files located on the folder pointed by **site.post_src_dir** on the **site.txt** file. 

Post file names should be named according to the rule:
```LAYOUT-YYYYMMDD-TITLE.txt```
where:
- LAYOUT is the name of the layout file used to render this post content. A matching layout html file is expected to exist at _site_root_/layout. For example if LAYOUT is "PostLayout", there must be a _site_root_/layout/PostLayout.html.
- YYYYMMDD is a timestamp where YYYY is a 4 digit year, MM is 2 digit month and DD is a 2 digit day.
- TITLE is the post title. 

Files with wrong naming convention will be ignored. 
Files with invalid month/day ranges will be ignored.
Files without a matching layout file will be ignored.

Posts can have anyting insde. Ideally you should add html to structure the post as needed.

## Layout files
Layout files should be placed at _site_root_/layout and are used to render posts.
The following "tags" can be used on a layout file to render post information:
- **{{post.title}}** The title of the post as specified on the post file name.
- **{{post.year}}** The publishing year of the post as specified on the post file name.
- **{{post.month}}** The publishing month of the post as specified on the post file name. This is a value between **01** and **12**
- **{{post.day}}** The publishing day of the post as specified on the post file name.
- **{{post.body}}** The full unprocessed content of the post file.
- **{{post.month_name}}** - The name of the publishing month of the post. This can be overriden on the site.txt file. This is a value between **01** and **12**
- **{{post.url}}** The post url
- **{{post.layout}}** The name of the layout used by this post as defined on the post file name.

## Page files
It's possible to have standalone pages unrelated to posts. Standalone pages are html files placed at _site_root_. Files with different extension are ignored.
The folowwing tags are available for Page files.
- **{{page.title}}** The page file name without extension. For example the {{page.title}} for page _site_root_/About.html will be "About".
- **{{page.url}}** The page file name without extension. For example the {{page.title}} for page _site_root_/About.html will be "About".

## General commands and tags
All keys in the _site.txt_ file are acessible from Pages and Layouts with the ```{{key}}``` syntax. For example, you can get the site name with ```{{site.name}}``` or the site.url with ```{{site.url}}```.

The folowwing global tags are available for Pages and Posts:
- **{{site.num_pages}}** The number of valid pages.
- **{{site.num_posts}}** The number of valid posts.

In addition to tags there are commands that can be used on Pages and Posts.

### Include command
The include command is used to insert the content of another file into the current page.

The syntax is:

```{{include PATH}}```

Path is a double-quoted path to the file to be included. 
The path is relative to _site_root_

The following line includes the contents of the file ** _site_root_/include/header.html**
```
{{include include/header.html}}
```

### For command
The For command allows iterating over collections and the syntax is:
```{{for ITERATORNAME in COLLECTION}} {{endfor}}```

Where:
- ITERATORNAME is any name (except reserved tag names) to use as the collection iterator.
  - Regardless of the collection, it's always possible to get the current iteration count from {{ITERATORNAME.number}}
- COLLECTION is the collection name. There are 2 only collections implemented at the moment: 
  - **all_posts** exposes all the post tags mentioned so far exept the {{post.body}} that is only avaliable from layout files (see Layout files above). The posts are sorted by date from newest to the oldest.
  - **all_pages** exposes all the page tags (see page files above).

Anything between the for/endfor command will be parsed and processed for as many times as items in the collection.
Any valid commands and tags can be used inside a for/endfor block. Be warned there is no recursivity check or limit implemented so far.

A simple menu with all the posts could be written like this:

```
<h2>Archive</h2>
  <nav>
    <ul class="archive-list">
      {{for p in all_posts}}
        <li><a href="{{p.url}}">{{p.title}}<p>
        <time datetime="{{p.year}}-{{p.month}}-{{p.day}}:00:00+02:00">
          {{p.year}}-{{p.month_name}}-{{p.day}}
        </time></p></a></li>
      {{endfor}}
    </ul>
  </nav>
```

## Conclusion
That's all I needed in terms of static site generation. I might extend this program in case I need something extra. 
If this is usefull to you feel free to use it or change it yourself.



 
 



 
