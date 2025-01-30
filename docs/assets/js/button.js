document.addEventListener("DOMContentLoaded", function () {
    const button = document.getElementById("custom-button");
  
    if (button) {
      button.addEventListener("click", function () {
        if (jtd.getTheme() === 'dark') {
            jtd.setTheme('light');
          } else {
            jtd.setTheme('dark');
          }
      });
    }
  });
  