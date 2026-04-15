import tkinter as tk
from tkinter import ttk, filedialog, font as tkfont
import re
from PIL import Image, ImageDraw, ImageFont, ImageTk

# ── helpers ──────────────────────────────────────────────────────────────────

def parse_input(raw: str) -> str:
    """
    Convert a mixed string like  hello U+E011 world U+E012
    into the real Unicode characters so the font can render them.
    """
    def replace_codepoint(m):
        return chr(int(m.group(1), 16))
    return re.sub(r'[Uu]\+([0-9A-Fa-f]{4,6})', replace_codepoint, raw)

# ── main app ─────────────────────────────────────────────────────────────────

class FontVisualizer(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Font Visualizer")
        self.geometry("860x620")
        self.configure(bg="#1e1e2e")
        self.resizable(True, True)

        self._font_path = None
        self._custom_font_name = None
        self._build_ui()
        self._refresh_system_fonts()

    # ── UI construction ───────────────────────────────────────────────────────

    def _build_ui(self):
        PAD = dict(padx=10, pady=6)
        BG  = "#1e1e2e"
        SBG = "#313244"
        FG  = "#cdd6f4"
        ACC = "#89b4fa"
        BTN = dict(bg=ACC, fg="#1e1e2e", activebackground="#74c7ec",
                   relief="flat", cursor="hand2", font=("Segoe UI", 9, "bold"),
                   padx=8, pady=4)

        # ── top bar ───────────────────────────────────────────────────────────
        top = tk.Frame(self, bg=BG)
        top.pack(fill="x", padx=12, pady=(12, 0))

        tk.Label(top, text="Font Visualizer", bg=BG, fg=ACC,
                 font=("Segoe UI", 14, "bold")).pack(side="left")

        tk.Button(top, text="Load Font File (.ttf/.otf)",
                  command=self._load_font, **BTN).pack(side="right")

        self.lbl_font = tk.Label(top, text="No custom font loaded",
                                 bg=BG, fg="#6c7086", font=("Segoe UI", 9))
        self.lbl_font.pack(side="right", padx=(0, 12))

        ttk.Separator(self, orient="horizontal").pack(fill="x", padx=12, pady=8)

        # ── controls row ──────────────────────────────────────────────────────
        ctrl = tk.Frame(self, bg=BG)
        ctrl.pack(fill="x", padx=12)

        # font selector
        tk.Label(ctrl, text="Font:", bg=BG, fg=FG,
                 font=("Segoe UI", 9)).grid(row=0, column=0, sticky="w", **PAD)
        self.var_font = tk.StringVar(value="TkDefaultFont")
        self.combo_font = ttk.Combobox(ctrl, textvariable=self.var_font, width=28,
                                       state="readonly")
        self.combo_font.grid(row=0, column=1, **PAD)
        self.combo_font.bind("<<ComboboxSelected>>", lambda e: self._update_preview())

        # size
        tk.Label(ctrl, text="Size:", bg=BG, fg=FG,
                 font=("Segoe UI", 9)).grid(row=0, column=2, sticky="w", **PAD)
        self.var_size = tk.IntVar(value=36)
        spin = tk.Spinbox(ctrl, from_=6, to=200, textvariable=self.var_size,
                          width=5, bg=SBG, fg=FG, insertbackground=FG,
                          buttonbackground=SBG, relief="flat",
                          command=self._update_preview)
        spin.grid(row=0, column=3, **PAD)
        spin.bind("<Return>", lambda e: self._update_preview())

        # bold / italic
        self.var_bold   = tk.BooleanVar()
        self.var_italic = tk.BooleanVar()
        tk.Checkbutton(ctrl, text="Bold",   variable=self.var_bold,
                       bg=BG, fg=FG, selectcolor=SBG, activebackground=BG,
                       command=self._update_preview).grid(row=0, column=4, **PAD)
        tk.Checkbutton(ctrl, text="Italic", variable=self.var_italic,
                       bg=BG, fg=FG, selectcolor=SBG, activebackground=BG,
                       command=self._update_preview).grid(row=0, column=5, **PAD)

        # fg / bg colour pickers
        tk.Label(ctrl, text="Text color:", bg=BG, fg=FG,
                 font=("Segoe UI", 9)).grid(row=0, column=6, sticky="w", **PAD)
        self.var_fg = tk.StringVar(value="#cdd6f4")
        self.btn_fg = tk.Button(ctrl, width=3, bg=self.var_fg.get(), relief="flat",
                                cursor="hand2", command=self._pick_fg)
        self.btn_fg.grid(row=0, column=7, **PAD)

        tk.Label(ctrl, text="BG:", bg=BG, fg=FG,
                 font=("Segoe UI", 9)).grid(row=0, column=8, sticky="w", **PAD)
        self.var_bg = tk.StringVar(value="#1e1e2e")
        self.btn_bg = tk.Button(ctrl, width=3, bg=self.var_bg.get(), relief="flat",
                                cursor="hand2", command=self._pick_bg)
        self.btn_bg.grid(row=0, column=9, **PAD)

        ttk.Separator(self, orient="horizontal").pack(fill="x", padx=12, pady=6)

        # ── input area ────────────────────────────────────────────────────────
        inp_frame = tk.Frame(self, bg=BG)
        inp_frame.pack(fill="x", padx=12)

        tk.Label(inp_frame,
                 text="Input text  (mix regular text & codepoints, e.g.:  Hello U+E011 World U+E012)",
                 bg=BG, fg="#6c7086", font=("Segoe UI", 8)).pack(anchor="w")

        input_box_frame = tk.Frame(inp_frame, bg=ACC, padx=1, pady=1)
        input_box_frame.pack(fill="x", pady=(4, 0))

        self.txt_input = tk.Text(input_box_frame, height=3, bg=SBG, fg=FG,
                                 insertbackground=FG, relief="flat",
                                 font=("Consolas", 11), wrap="word",
                                 padx=8, pady=6)
        self.txt_input.pack(fill="x")
        self.txt_input.insert("1.0", "Hello World  U+E011 U+E012")
        self.txt_input.bind("<KeyRelease>", lambda e: self._update_preview())

        # ── preview canvas ────────────────────────────────────────────────────
        ttk.Separator(self, orient="horizontal").pack(fill="x", padx=12, pady=8)

        tk.Label(self, text="Preview", bg=BG, fg=ACC,
                 font=("Segoe UI", 10, "bold")).pack(anchor="w", padx=14)

        canvas_frame = tk.Frame(self, bg=SBG, bd=0)
        canvas_frame.pack(fill="both", expand=True, padx=12, pady=(4, 12))

        self.canvas = tk.Canvas(canvas_frame, bg="#1e1e2e", bd=0,
                                highlightthickness=0)
        self.canvas.pack(fill="both", expand=True, padx=2, pady=2)

        # ── status bar ────────────────────────────────────────────────────────
        self.lbl_status = tk.Label(self, text="", bg="#181825", fg="#6c7086",
                                   font=("Consolas", 8), anchor="w")
        self.lbl_status.pack(fill="x", padx=0, pady=0)

        self._update_preview()

    # ── font loading ──────────────────────────────────────────────────────────

    def _load_font(self):
        path = filedialog.askopenfilename(
            title="Open Font File",
            filetypes=[("Font files", "*.ttf *.otf"), ("All files", "*.*")]
        )
        if not path:
            return
        try:
            from tkinter import font as tkfont
            # pyglet gives us the cleanest way to load a font into Tk on all OSes
            try:
                import pyglet
                pyglet.font.add_file(path)
                import re as _re
                name_guess = _re.sub(r'[-_]', ' ',
                                     path.split("/")[-1].rsplit(".", 1)[0])
                self._custom_font_name = name_guess
            except ImportError:
                # fallback: try to register via Windows-only CTkFont trick or just
                # use the filename stem as the family name after adding via pyglet
                import ctypes, os
                if os.name == "nt":
                    FR_PRIVATE = 0x10
                    ctypes.windll.gdi32.AddFontResourceExW(path, FR_PRIVATE, 0)
                    import re as _re
                    self._custom_font_name = _re.sub(
                        r'[-_]', ' ', path.split("\\")[-1].rsplit(".", 1)[0])
                else:
                    raise RuntimeError("pyglet not available; install it with: pip install pyglet")

            self._font_path = path
            short = path.split("/")[-1].split("\\")[-1]
            self.lbl_font.config(text=f"✓ {short}", fg="#a6e3a1")
            self._refresh_system_fonts(prepend=self._custom_font_name)
            self.var_font.set(self._custom_font_name)
            self._update_preview()
        except Exception as e:
            self.lbl_font.config(text=f"Error: {e}", fg="#f38ba8")

    def _refresh_system_fonts(self, prepend=None):
        families = sorted(tkfont.families())
        if prepend:
            families = [prepend] + [f for f in families if f != prepend]
        self.combo_font["values"] = families

    # ── colour pickers ────────────────────────────────────────────────────────

    def _pick_fg(self):
        from tkinter import colorchooser
        c = colorchooser.askcolor(color=self.var_fg.get(), title="Text Color")
        if c[1]:
            self.var_fg.set(c[1])
            self.btn_fg.config(bg=c[1])
            self._update_preview()

    def _pick_bg(self):
        from tkinter import colorchooser
        c = colorchooser.askcolor(color=self.var_bg.get(), title="Background Color")
        if c[1]:
            self.var_bg.set(c[1])
            self.btn_bg.config(bg=c[1])
            self.canvas.config(bg=c[1])
            self._update_preview()

    # ── preview rendering ─────────────────────────────────────────────────────

    def _update_preview(self):
        raw  = self.txt_input.get("1.0", "end-1c")
        text = parse_input(raw)

        size   = self.var_size.get()
        fg     = self.var_fg.get()
        bg     = self.var_bg.get()

        self.canvas.delete("all")
        self.canvas.config(bg=bg)

        cw = self.canvas.winfo_width()  or 800
        ch = self.canvas.winfo_height() or 300

        # load PIL font — only works if a real file path is loaded
        pil_font = None
        if self._font_path:
            try:
                pil_font = ImageFont.truetype(self._font_path, size)
            except Exception:
                pass
        if pil_font is None:
            try:
                pil_font = ImageFont.load_default(size=size)
            except Exception:
                pil_font = ImageFont.load_default()

        # measure text
        dummy = Image.new("RGBA", (1, 1))
        dd    = ImageDraw.Draw(dummy)
        bbox  = dd.textbbox((0, 0), text, font=pil_font)
        tw    = bbox[2] - bbox[0]
        th    = bbox[3] - bbox[1]

        # render onto correctly-sized image centered in canvas
        img = Image.new("RGBA", (cw, ch), self._hex_to_rgba(bg))
        d   = ImageDraw.Draw(img)
        x   = (cw - tw) // 2 - bbox[0]
        y   = (ch - th) // 2 - bbox[1]
        d.text((x, y), text, font=pil_font, fill=self._hex_to_rgba(fg))

        self._tk_img = ImageTk.PhotoImage(img)
        self.canvas.create_image(0, 0, anchor="nw", image=self._tk_img)

        # status bar
        codepoints = " ".join(f"U+{ord(c):04X}" for c in text if ord(c) > 127)
        self.lbl_status.config(
            text=f"  chars: {len(text)}   non-ASCII: {codepoints or '—'}   "
                 f"rendered size: {size}px   text size: {tw}×{th}px"
        )

        self.canvas.bind("<Configure>", lambda e: self._update_preview())

    def _hex_to_rgba(self, hex_color: str):
        hex_color = hex_color.lstrip("#")
        r, g, b = int(hex_color[0:2], 16), int(hex_color[2:4], 16), int(hex_color[4:6], 16)
        return (r, g, b, 255)


# ── entry point ───────────────────────────────────────────────────────────────

if __name__ == "__main__":
    app = FontVisualizer()
    app.mainloop()