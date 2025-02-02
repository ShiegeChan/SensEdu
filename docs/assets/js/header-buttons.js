const gitIcon = document.getElementById("git-logo");
const themeIcon = document.getElementById("theme-logo");
const gitButton = document.getElementById("git-button");
const themeButton = document.getElementById("theme-button");

const DARK_THEME = "dark";
const LIGHT_THEME = "light";

function openGitHub() {
    window.open('https://github.com/ShiegeChan/SensEdu','_blank');
    gitButton.blur(); // remove button focus
}

function changeTheme() {
    const currentTheme = jtd.getTheme();
    if (currentTheme === DARK_THEME) {
        applyTheme(LIGHT_THEME);
    } else {
        applyTheme(DARK_THEME);
    }
}

function applyTheme(new_theme) {
    jtd.setTheme(new_theme);

    if (new_theme === DARK_THEME) {
        gitIcon.src = "/assets/images/github-logo-light.png";
        themeIcon.src = "/assets/images/sun.png";

        gitButton.className = 'header-button hb_dark';
        themeButton.className = 'header-button hb_dark';
    } else {
        gitIcon.src = "/assets/images/github-logo-dark.png";
        themeIcon.src = "/assets/images/moon.png";

        gitButton.className = 'header-button hb_light';
        themeButton.className = 'header-button hb_light';
    }

    themeButton.blur(); // remove button focus
    saveTheme(new_theme, gitIcon.src, themeIcon.src, gitButton.className, themeButton.className);
}

function saveTheme(new_theme, git_icon_src, theme_icon_src, git_button_class, theme_button_class) {
    localStorage.setItem("theme", new_theme);
    localStorage.setItem("git-icon", git_icon_src);
    localStorage.setItem("theme-icon", theme_icon_src);
    localStorage.setItem("git-button-class", git_button_class);
    localStorage.setItem("theme-button-class", theme_button_class);
}

document.addEventListener("DOMContentLoaded", function () {

    // load all data from local storage
    // because these settings are not global for each page
    // for some magical reason
    const savedTheme = localStorage.getItem("theme");
    if (savedTheme == DARK_THEME || savedTheme == LIGHT_THEME) {
        jtd.setTheme(savedTheme);

        const savedGitIconSrc = localStorage.getItem("git-icon");
        const savedThemeIconSrc = localStorage.getItem("theme-icon");
        const savedGitButtonClass = localStorage.getItem("git-button-class");
        const savedThemeButtonClass = localStorage.getItem("theme-button-class");
        
        gitIcon.src = savedGitIconSrc;
        themeIcon.src = savedThemeIconSrc;
        gitButton.className = savedGitButtonClass;
        themeButton.className = savedThemeButtonClass;
    }

});



