---
title: Projects
layout: default
parent: Contributing
nav_order: 3
---

# Project Contributions
{: .fs-8 .fw-500 .no_toc}
---

We are excited to see the innovative ways in which you are utilizing SensEdu. By submitting your project, you have the opportunity to have it listed as a showcase for SensEdu functionality and included in the main repository.
{: .fw-500}

To submit your project, please follow these steps:
1. Fork [SensEdu repository]
2. Add a new project folder in your fork `~\projects\my_project\`
3. Create an Arduino sketch `~\projects\my_project\my_project.ino`. Ensure that the sketch filename matches the folder name
   1. If you have additional files (e.g., MATLAB scripts), place them in the  subfolder within your project folder, such as `~\projects\my_project\matlab\my_script.m`
4. Create a documentation for your project
   1. Create a new markdown page `~\docs\Projects\my_project.md`
   2. Add the following fields to the markdown header:
   ```md
   ---
   title: My Project
   layout: default
   parent: Projects
   nav_order: 10
   ---
   ```
   3. The order of projects is defined by `nav_order`. Use the next available number for proper website navigation. Customize `title` with your project name
   4. Follow [Documentation Contributions]({% link Contributing/docs.md %}) for detailed page creation guidelines
   5. Share implementation details and nice pictures of you project!
5. Commit all changes to your fork and submit a [Pull Request] (PR) to the main SensEdu [repository]

[repository]: https://github.com/ShiegeChan/SensEdu
[SensEdu repository]: https://github.com/ShiegeChan/SensEdu
[Pull Request]: https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request
