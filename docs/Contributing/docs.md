---
title: Documentation
layout: default
parent: Contributing
nav_order: 1
---

# Documentation Contributions
{: .fs-8 .fw-500 .no_toc}
---

We highly appreciate any suggestions for improvements to enhance the clarity of our documentation. Documentation is written using Markdown files that are rendered and transformed into a static website using [Jekyll] and its template [Just the Docs].
{: .fw-500}

- TOC
{:toc}

### Add New Pages

Each respective tab on the website has its corresponding folder (e.g., `~\docs\Library\` for the library section). In this folder, there is always an `index.md` file that corresponds to the main page for this tab. Each subpage can be named arbitrarily (e.g., `page.md`) and must reference the parent page in the file header using the `parent` field.

```md
---
title: new page
layout: default
parent: Library
nav_order: 10
---
```

* `title`: Page title
* `layout`: Keep this at `default`
* `parent`: Reference the parent page by name for subpages
* `nav_order`: Defines the subpage order. 2 appears higher than 3 in navigation

You can add an optional `math: mathjax` line to enable [MathJax] syntax on this page.

### Syntax

If you have any images, place them into the `~\docs\assets\images\` folder and reference them with `{{site.baseurl}}`. Below is an example with centering and width adjustment.

```md
<img src="{{site.baseurl}}/assets/images/my_picture.png" alt="drawing" width="500"/>
{: .text-center}
```

Other syntax is standard for Markdown with modifiers added by Just the Docs. Follow these pages to explore the syntax further: 
* <a href="https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax" target="_blank">Basic writing and formatting syntax</a>
* <a href="https://just-the-docs.com/" target="_blank">Just the Docs page</a>

### Wiki Hosting

When you edit the wiki, it is advised to observe your changes on the finished rendered webpage. You can host it locally by following these steps:

0. Administrator rights may be required to install Ruby and its gems.
1. Visit the <a href="https://rubyinstaller.org/downloads/" target="_blank">Ruby installation page</a>. Download the **x64 version with devkit**.
During installtion you will be asked which components to install, press `Enter` for default.
2. Open the terminal with admin rights in `/docs` folder.
3. Install gems with `bundle install` command.
4. Boot the website with `bundle exec jekyll serve --livereload`. Parameter
`--livereload` is optional, it enables automatic website reloading if you make any changes to styles/text etc.
5. Go to the page `localhost:4000/SensEdu/` in your browser to see the website.

#### Notes:
{: .no_toc}
* Stop the running website with `Ctrl+C` in the terminal.
* If you modify `_config.yml`, restart the page (even if `--livereload` enabled).

<img src="{{site.baseurl}}/assets/images/readme_docs.png" alt="drawing"/>
{: .text-center}

[Jekyll]: https://jekyllrb.com/
[Just the Docs]: https://just-the-docs.com/
[MathJax]: https://www.mathjax.org/