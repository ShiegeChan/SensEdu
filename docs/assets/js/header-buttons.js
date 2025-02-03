const themeSwitcher = document.querySelector('.js-theme-switcher');
const openGitHub = document.querySelector('.js-git-open');

const gitButton = document.getElementById("git-button");
const themeButton = document.getElementById("theme-button");

jtd.addEvent(themeSwitcher, 'click', function() {
    if (jtd.getTheme() === 'dark') {
        jtd.setTheme('default');
        gitButton.className = 'header-button hb_light js-git-open';
        themeButton.className = 'header-button hb_light js-theme-switcher';
    } else {
        jtd.setTheme('dark');
        gitButton.className = 'header-button hb_dark js-git-open';
        themeButton.className = 'header-button hb_dark js-theme-switcher';
    }

    // remove button focus
    themeButton.blur(); 
});

jtd.addEvent(openGitHub, 'click', function() {
    window.open('https://github.com/ShiegeChan/SensEdu','_blank');
    gitButton.blur(); 
});

document.addEventListener("DOMContentLoaded", function () {
    if (jtd.getTheme() === 'dark') {
        gitButton.className = 'header-button hb_dark js-git-open';
        themeButton.className = 'header-button hb_dark js-theme-switcher';
    }
});