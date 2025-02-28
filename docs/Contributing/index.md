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

If you are still not sure, feel free to open a [discussion](https://github.com/ShiegeChan/SensEdu/discussions) and we'll find a job for you!

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

1. Create an issue with the `enhancement` label:
   1. Descibe your idea in detail
   2. Provide some usage examples

### Improve Documentation
Clear documentation is essential for our educational purposes, help us:
* Fix typos or grammatical errors
* Clarify ambiguous explanations to ensure everything is easy to understand
* Fill in missing explanations

{: .warning}
The documentation is targeted to beginner students as well. Try to avoid logical skips, use simple language and add diagrams, image or code examples!

### Improve Code

1. Review the [library wiki section]({% link Library/index.md %}) and [naming convention]({% link Contributing/index.md %}#library-contributions)
2. Assign yourself an [issue] or [discussion]
3. Create a Pull Request (PR) and ask for a review

{: .warning}
Try to keep PRs small, focusing on one feature/fix

### Submit Project
Created something amazing using SensEdu? Share it with us! Check [Project Submissions]({% link Contributing/index.md %}#project-submissions) for detailed instructions.


## Library Contributions

Please follow naming convention used throught the code. Some of it described in [library main page]({% link Library/index.md %}). Be careful with usign new timers and dma streams, make sure they are free. Read through developers notes for each peripheral you want to modify.

If you are an experienced embedded systems developer, feel free to optimize library code for better performance.

{: .warning}
Before pushing your library changes, make sure that **ALL** examples are working properly


## Project Submissions

If you have developed any projects using our shield, please feel free to push it to `projects\your_project\` directory. Additional examples for usage are much appreciated even if they are quite similar to already developed projects.

If you are not sure, which project to develop there are some suggestions:
* FSK (Frequency-Shift Keying) modulated communication between multiple boards
* Radar speed gun
* Weather station with barometric pressure sensor


## Roadmap

- [x] DAC Channel 2 support
- [x] Automatic buffer alignment for DAC
- [x] Cache coherence issues with DAC
- [x] ADC3 support
- [ ] Automatic buffer alignment for ADC
- [ ] Rewrite ADC sampling frequency calculations
- [ ] Separate timers for each ADC and DAC channel
- [ ] Ultrasonic measurements performance boost using DMA
- [ ] XCorr calibration algorithm
- [ ] Improved Arduino-MATLAB serial communication
- [ ] SensEdu library barometric sensor support
- [ ] Examples for barometric pressure usage
- [ ] 2-3 Fully developed projects as SensEdu showcase

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