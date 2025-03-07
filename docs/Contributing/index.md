---
title: Contributing
layout: default
nav_order: 7
permalink: /contributing/
---

# Contributing
{: .fs-8 .fw-500 .no_toc}
---

Whether you're fixing bugs, adding new functionality, improving documentation, or creating your own projects, your contribution is highly valued and appreciated!
{: .fw-500}

- TOC
{:toc}

## Getting Started
Before diving in, familiarize yourself with SensEdu:
1.  Read through this wiki
2.  Run basic examples and finished projects
3.  Explore existing [issues] and [roadmap]({%link Contributing/index.md %}#roadmap)

If you are still not sure what to do, feel free to open a [discussion](https://github.com/ShiegeChan/SensEdu/discussions) and we'll find a task for you!

## How to Contribute

### Report Bugs
Found a bug or unexpected behavior?  Help us fix it:
1. Search for existing [issue] to avoid duplicates
2. Create a new [issue] with `bug` label, providing:
   1. Steps to reproduce
   2. Expected vs actual behaviour
   3. Screenshots, logs, oscilloscope waveforms or any additional data

### Suggest Features
Have an idea for a new sensor driver, project or optimization? 

1. Create an [issue] with the `enhancement` label:
   1. Descibe your idea in detail
   2. Provide some usage examples

### Improve Documentation
Clear documentation is essential for our educational purposes, help us:
* Fix typos or grammatical errors
* Clarify ambiguous explanations to ensure everything is easy to understand
* Fill in missing explanations

Check [Documentation Contributions]({% link Contributing/docs.md %}) for detailed instructions.

{: .note}
For educational purposes, the documentation is aimed not only at developers but at beginner students as well. Try to avoid logical skips, use simple language and add diagrams, image or code examples!

### Improve Code

If you are an experienced embedded systems developer, feel free to optimize library code for better performance.

1. Review the [library wiki section]({% link Library/index.md %}) and [naming convention]({% link Contributing/index.md %}#library-contributions)
2. Assign yourself an [issue] to ensure collaboration with others who are working on the same feature and to avoid duplicates
3. Create a fork and develop your feature
4. Submit a [Pull Request] (PR) and ask for a review

{: .note}
Try to keep PRs small, focusing on one feature/fix per PR.

### Submit Project
Created something amazing using SensEdu? Share it with us! Check [Project Contributions]({% link Contributing/index.md %}#project-contributions) for detailed instructions.

Projects are much appreciated even if they are quite similar to already developed ones. If you are unsure which project to develop, here are some suggestions:
* Radar speed gun
* FSK (Frequency-Shift Keying) modulated communication between multiple boards
* Weather station with barometric pressure sensor


## Roadmap

- [ ] Automatic buffer alignment for ADC
- [ ] Separate timers for each ADC and DAC channel
- [ ] Ultrasonic measurements performance boost using DMA
- [ ] Xcorr calibration algorithm
- [ ] Improved Arduino-MATLAB serial communication
- [ ] Barometric sensor support
- [ ] FMCW Radar

---

Thank You to All Contributors!
{: .fw-500 .mb-1}

<ul class="list-style-none">
{% for contributor in site.github.contributors %}
  <li class="d-inline-block mr-1">
     <a href="{{ contributor.html_url }}"><img src="{{ contributor.avatar_url }}" width="32" height="32" alt="{{ contributor.login }}"></a>
  </li>
{% endfor %}
</ul>

[issue]: https://github.com/ShiegeChan/SensEdu/issues
[issues]: https://github.com/ShiegeChan/SensEdu/issues
[discussion]: https://github.com/ShiegeChan/SensEdu/discussions

