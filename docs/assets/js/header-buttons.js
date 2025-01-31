function openGitHub() {
    window.open('https://github.com/ShiegeChan/SensEdu','_blank');
    
    const gitHubButton = document.getElementById("github-button");
    gitHubButton.blur(); // remove button focus
}

function changeTheme() {
    const currentTheme = jtd.getTheme();
    const newTheme = currentTheme === "dark" ? "light" : "dark";
    applyTheme(newTheme);
}

function applyTheme(new_theme) {
    jtd.setTheme(new_theme);

    const gitHubButton = document.getElementById("github-button");
    const themeButton = document.getElementById("theme-toggle-button");
    const gitHubIcon = document.getElementById("github-logo");
    const themeIcon = document.getElementById("theme-logo");

    if (new_theme === "dark") {
        gitHubButton.className = 'button-dark';
        themeButton.className = 'button-dark';

        gitHubIcon.src = "/assets/images/github-logo-light.png";
        themeIcon.src = "/assets/images/sun.png";
        
    } else {
        gitHubButton.className = 'button-light';
        themeButton.className = 'button-light';

        gitHubIcon.src = "/assets/images/github-logo-dark.png";
        themeIcon.src = "/assets/images/moon.png";
    }

    themeButton.blur(); // remove button focus

    saveTheme(new_theme, gitHubIcon.src, themeIcon.src, gitHubButton.className, themeButton.className);
}

function saveTheme(new_theme, git_icon_src, theme_icon_src, git_button_class, theme_button_class) {
    localStorage.setItem("theme", new_theme);
    localStorage.setItem("git-icon", git_icon_src);
    localStorage.setItem("theme-icon", theme_icon_src);
    localStorage.setItem("git-button", git_button_class);
    localStorage.setItem("theme-button", theme_button_class);
}

document.addEventListener("DOMContentLoaded", function () {
    // load all data from local storage
    // because these settings are not global for each page
    // for some magical reason
    const savedTheme = localStorage.getItem("theme");
    if (savedTheme) {
        const savedGitIconSrc = localStorage.getItem("git-icon");
        const savedThemeIconSrc = localStorage.getItem("theme-icon");
        const savedGitButtonClass = localStorage.getItem("git-button");
        const savedThemeButtonClass = localStorage.getItem("theme-button");

        const gitHubIcon = document.getElementById("github-logo");
        const themeIcon = document.getElementById("theme-logo");
        const gitHubButton = document.getElementById("github-button");
        const themeButton = document.getElementById("theme-toggle-button");
        
        gitHubIcon.src = savedGitIconSrc;
        themeIcon.src = savedThemeIconSrc;
        gitHubButton.className = savedGitButtonClass;
        themeButton.className = savedThemeButtonClass;

        jtd.setTheme(savedTheme);
    }
  });
