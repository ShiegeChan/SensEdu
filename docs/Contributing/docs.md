---
title: Documentation
layout: default
parent: Contributing
nav_order: 1
---

# Documentation Contributions
{: .fs-8 .fw-500 .no_toc}
---

Documentation is written with markdown files that are rendered and transformed into static website using [Jekyll] and its template [Just the Docs].
{: .fw-500}

- TOC
{:toc}

### Add new pages

Each respective tab in website has its corresponding folder (e.g., `~\docs\Library\` for library). In this folder there is always `index.md` file which is corresponding to the main page for this tab. Then each other subpage could be named anyway `page.md` and must be refferenced to the parent page in file header in `parent` field.
```md
---
title: new page
layout: default
parent: Library
nav_order: 10
---
```

* `title`: Page title
* `layout`: keep it at `default`
* `parent`: reference parent page by name for subpages
* `nav_order`: defines subpage order, 2 appears higher then 3 in navigation

You can add optional `math: mathjax` line to enable mathjax syntax in this page.

### Syntax

If you have any pictures, put them into `~\docs\assets\images\` folder and reference them with `{{site.baseurl}}`, example below with centering and width adjustment:
```md
<img src="{{site.baseurl}}/assets/images/my_picture.png" alt="drawing" width="500"/>
{: .text-center}
```

Other syntax is standard for markdown with added by just-the-docs modifyers. Follow these pages to explore syntax further: <a href="https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax" target="_blank">basic writing and formatting syntax</a> and <a href="https://just-the-docs.com/" target="_blank">Just the Docs page</a>.

### Wiki hosting

When you edit the wiki it is advised to observe your changes on finished rendered webpage. You can host it locally following these steps:

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
[mathjax]: https://www.mathjax.org/