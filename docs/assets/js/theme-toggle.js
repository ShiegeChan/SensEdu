document.addEventListener("DOMContentLoaded", function () {
    // Function to apply the saved theme
    function applyTheme(theme) {
        jtd.setTheme(theme);
    }

    // Update icon
    function updateIcon(theme, git_icon, theme_icon) {
        if (theme === "dark") {
            theme_icon.src = "/assets/images/sun.png";
            theme_icon.style.height = '30px';
            git_icon.src = "/assets/images/github-logo-light.png";
            git_icon.style.height = '35px';
        } else {
            theme_icon.src = "/assets/images/moon.png";
            theme_icon.style.height = '30px';
            git_icon.src = "/assets/images/github-logo-dark.png";
            git_icon.style.height = '35px';
        }
    }

    // Load the theme from localStorage
    const savedTheme = localStorage.getItem("theme");
    if (savedTheme) {
        applyTheme(savedTheme);

        // Retrieve stored icon src URLs from localStorage
        const savedGitHubIcon = localStorage.getItem("git_icon");
        const gitHubIcon = document.getElementById("github-logo");
        gitHubIcon.src = savedGitHubIcon;

        const savedThemeIcon = localStorage.getItem("theme_icon");
        const themeIcon = document.getElementById("theme-logo");
        themeIcon.src = savedThemeIcon;
    }

    // Add event listener to toggle button
    const button = document.getElementById("theme-toggle-button");
    if (button) {
        button.addEventListener("click", function (event) {
            event.preventDefault(); // Prevents aux-link

            const currentTheme = jtd.getTheme();
            const newTheme = currentTheme === "dark" ? "light" : "dark";
            
            applyTheme(newTheme);

            const gitHubIcon = document.getElementById("github-logo");
            const themeIcon = document.getElementById("theme-logo");
            updateIcon(newTheme, gitHubIcon, themeIcon);

            // Save theme and icon src URLs to localStorage
            localStorage.setItem("theme", newTheme);
            localStorage.setItem("git_icon", gitHubIcon.src);  // Save src of GitHub icon
            localStorage.setItem("theme_icon", themeIcon.src); // Save src of theme icon
        });
    }
  });
