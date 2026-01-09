/* Smart Lower Thirds Landing — small UX helpers
   - Smooth scrolling for in-page links
   - Mobile menu toggle
   - Tabbed video switcher
*/
(() => {
  // Smooth scrolling
  function onAnchorClick(e) {
    const a = e.target.closest('a[href^="#"]');
    if (!a) return;

    const href = a.getAttribute("href") || "";
    const id = href.replace(/^#/, "");
    if (!id) return;

    const el = document.getElementById(id);
    if (!el) return;

    e.preventDefault();
    el.scrollIntoView({ behavior: "smooth", block: "start" });

    // Close mobile nav if open
    const mobile = document.querySelector("[data-mobilenav]");
    const btn = document.querySelector("[data-navbtn]");
    if (mobile && btn && !mobile.hasAttribute("hidden")) {
      mobile.setAttribute("hidden", "");
      btn.setAttribute("aria-expanded", "false");
    }
  }
  document.addEventListener("click", onAnchorClick, { passive: false });

  // Mobile menu
  const btn = document.querySelector("[data-navbtn]");
  const mobile = document.querySelector("[data-mobilenav]");
  if (btn && mobile) {
    btn.addEventListener("click", () => {
      const open = !mobile.hasAttribute("hidden");
      if (open) {
        mobile.setAttribute("hidden", "");
        btn.setAttribute("aria-expanded", "false");
      } else {
        mobile.removeAttribute("hidden");
        btn.setAttribute("aria-expanded", "true");
      }
    });
  }

  // Video tabs
  const tabs = Array.from(document.querySelectorAll("[data-video-id]"));
  const iframe = document.querySelector("[data-tabbed-video]");
  const caption = document.querySelector("[data-video-caption]");

  const CAPTIONS = {
    "GtM2ytRJVEg": "Core walkthrough: templates, browser source selector, and the new local-file artifact workflow.",
    "AunKJCyrSmM?si=BMbtyXgfzxCMtJEx": "Demo / overview: general configuration and default lower thirds.",
    "iVcTWORlmJk": "Extra tips: additional setup and quality-of-life improvements."
  };

  function setTab(activeTab) {
    if (!iframe) return;
    const id = activeTab.getAttribute("data-video-id");
    if (!id) return;

    tabs.forEach(t => {
      const on = (t === activeTab);
      t.classList.toggle("is-active", on);
      t.setAttribute("aria-selected", on ? "true" : "false");
    });

    // Swap src and stop previous playback
    iframe.setAttribute("src", "https://www.youtube.com/embed/" + id);
    iframe.setAttribute("title", activeTab.textContent.trim() || "Smart Lower Thirds video");

    if (caption) caption.textContent = CAPTIONS[id] || "";
  }

  tabs.forEach(t => t.addEventListener("click", () => setTab(t)));

  // Default aria roles (defensive)
  tabs.forEach(t => {
    if (!t.hasAttribute("role")) t.setAttribute("role", "tab");
    if (!t.hasAttribute("aria-selected")) t.setAttribute("aria-selected", t.classList.contains("is-active") ? "true" : "false");
  });
})();
