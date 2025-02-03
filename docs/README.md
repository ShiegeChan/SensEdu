## How to host wiki locally on Windows

0. Administrator rights may be required to install Ruby and its gems.
1. Visit the <a href="https://rubyinstaller.org/downloads/">Ruby installation page</a>. Download the **x64 version with devkit**.
During installtion you will be asked which components to install, press `Enter` for default.
2. Open the terminal with admin rights in `/docs` folder.
3. Install gems with `bundle install` command.
4. Boot the website with `bundle exec jekyll serve`.
You can put optional `--livereload` parameter to automatically reload the website if you make some changes to styles/text etc.
5. Go to the page `localhost:4000` in your browser to see the website.

#### Notes:
* Stop the running website with `Ctrl+C` in the terminal.
* If you modify `_config.yml`, restart the page (even if `--livereload` enabled).

<p align="center" style="margin:0">
  <img src="https://ik.imagekit.io/vladysor/SensEdu/readme_docs.png?updatedAt=1738590979641" width="100%">
</p>