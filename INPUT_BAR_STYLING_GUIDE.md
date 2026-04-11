# Input Bar Styling Guide

## Overview
The entire input bar is now **100% controlled by the QSS stylesheet**. All C++ code has been removed for sizing, spacing, padding, colors, and layout.

## What You Can Tweak

Everything for the input bar is in `main/styles/stylesheet.qss` starting around line 256.

### Container (The whole input bar) ⭐ FULLY CUSTOMIZABLE
```qss
QWidget#inputBarOuter {
    /* BACKGROUND & BORDERS */
    background: %50;                    /* Background color (DARK_BG) */
    border-bottom: 1px solid %51;       /* Bottom border */
    border-top: 1px solid #333;         /* You can add top border too */
    border-left: 1px solid #333;        /* Add left border */
    border-right: 1px solid #333;       /* Add right border */
    border-radius: 0px;                 /* Rounded corners (0 = no rounding) */
    
    /* SPACING & SIZING */
    padding: 12px 12px;                 /* Inner padding (top/bottom left/right) */
    margin: 0px;                        /* Outer margin */
    min-height: 56px;                   /* Minimum height */
    max-height: 200px;                  /* Maximum height (optional) */
    
    /* APPEARANCE */
    min-width: 100%;                    /* Full width */
}
```

#### What Each Property Does:

| Property | Description | Examples |
|---|---|---|
| `background` | Fill color of the container | `#1a1a2e`, `#ffffff`, `qlineargradient(...)` |
| `border-bottom` | Bottom border line | `2px solid #666`, `3px dashed #fff` |
| `border-top` | Top border line | `1px solid #333` |
| `border-left` | Left border line | `1px solid #333` |
| `border-right` | Right border line | `1px solid #333` |
| `border-radius` | Corner rounding | `0px` (square), `4px`, `8px` |
| `padding` | Space *inside* container | `12px 12px` (top/bottom left/right) or `10px 15px 10px 15px` (all 4 sides) |
| `margin` | Space *outside* container | `0px`, `10px` |
| `min-height` | Minimum container height | `40px`, `60px`, `100px` |
| `max-height` | Maximum container height | `200px` (optional) |

### Mode Button (SCRAMBLE/ALG/POSITION)
```qss
QPushButton#btnInputMode {
    background: %64;              /* Background color */
    border: 1px solid %65;        /* Border color */
    border-radius: 4px 0 0 4px;   /* Rounded corners (left side only) */
    color: #fff;                  /* Text color */
    padding: 0 10px;              /* Inner padding */
    font-size: 11px;              /* Font size */
    font-weight: bold;            /* Font weight */
    min-height: 32px;             /* Height */
    min-width: 82px;              /* Width */
    max-width: 82px;              /* Lock width */
}

QPushButton#btnInputMode:hover {
    background: %66;              /* Hover color */
    border-color: %65;
}
```

### Dropdown Arrow
```qss
QPushButton#btnInputModeArrow {
    background: %67;              /* Background color */
    border: 1px solid %68;        /* Border color */
    border-radius: 0 4px 4px 0;   /* Rounded corners (right side only) */
    color: #fff;                  /* Text color */
    padding: 0 6px;               /* Inner padding */
    font-size: 11px;              /* Font size */
    min-height: 32px;             /* Height */
    min-width: 20px;              /* Width */
    max-width: 20px;              /* Lock width */
}

QPushButton#btnInputModeArrow:hover {
    background: %69;              /* Hover color */
    border-color: %68;
}
```

### Input Field
```qss
QLineEdit#txtMainInput {
    border-radius: 4px;           /* Rounded corners */
    border: 1px solid %73;        /* Border color */
    margin-left: 6px;             /* Space from arrow button */
    margin-right: 0px;
    font-family: monospace;       /* Font family */
    font-size: 12px;              /* Font size */
    background: %74;              /* Background color */
    color: %75;                   /* Text color */
    padding: 4px 8px;             /* Inner padding */
    min-height: 32px;             /* Height */
    max-height: 32px;             /* Lock height */
}

QLineEdit#txtMainInput[hasError="true"] {
    border-color: %39;            /* Error state border color */
}
```

### Apply Button
```qss
QPushButton#btnApply {
    background: %70;              /* Background color */
    border: 1px solid %71;        /* Border color */
    border-radius: 4px;           /* Rounded corners */
    margin-left: 8px;             /* Space from input field */
    color: #fff;                  /* Text color */
    font-size: 11px;              /* Font size */
    font-weight: bold;            /* Font weight */
    padding: 0 12px;              /* Inner padding */
    min-width: 64px;              /* Width */
    max-width: 64px;              /* Lock width */
    min-height: 32px;             /* Height */
    max-height: 32px;             /* Lock height */
}

QPushButton#btnApply:hover {
    background: %72;              /* Hover color */
    border-color: %71;
}
```

## Color Placeholders

The `%NN` values correspond to colors defined in `stylesheet.cpp`:

| Placeholder | Purpose | Color Theme |
|---|---|---|
| %50 | Input bar background | DARK_BG |
| %51 | Input bar border | BORDER_BOTTOM |
| %64 | Mode button background | INPUT_MODE_BG |
| %65 | Mode button border | INPUT_MODE_BORDER |
| %66 | Mode button hover | INPUT_MODE_HOVER |
| %67 | Arrow button background | INPUT_ARROW_BG |
| %68 | Arrow button border | INPUT_ARROW_BORDER |
| %69 | Arrow button hover | INPUT_ARROW_HOVER |
| %70 | Apply button background | INPUT_APPLY_BG |
| %71 | Apply button border | INPUT_APPLY_BORDER |
| %72 | Apply button hover | INPUT_APPLY_HOVER |
| %73 | Input field border | INPUT_FIELD_BORDER |
| %74 | Input field background | INPUT_FIELD_BG |
| %75 | Input field text | INPUT_FIELD_TEXT |
| %39 | Error state border | TEXT_ERROR |

## Examples

### Example 1: Make the input bar taller with more padding
```qss
QWidget#inputBarOuter {
    min-height: 70px;              /* Was 56px */
    padding: 16px 12px;            /* Was 12px 12px - more top/bottom space */
    background: %50;
    border-bottom: 1px solid %51;
}

QPushButton#btnInputMode,
QPushButton#btnInputModeArrow,
QLineEdit#txtMainInput,
QPushButton#btnApply {
    min-height: 40px;              /* Was 32px - taller buttons */
    max-height: 40px;
}
```

### Example 2: Dark input bar with a different background color
```qss
QWidget#inputBarOuter {
    background: #0a0a1a;           /* Darker background */
    border-bottom: 2px solid #2db570;  /* Thicker, green border */
    padding: 12px 12px;
    min-height: 56px;
}
```

### Example 3: Input bar with rounded corners and margin
```qss
QWidget#inputBarOuter {
    background: %50;
    border: 1px solid #444;        /* All borders, not just bottom */
    border-radius: 8px;            /* Rounded corners */
    padding: 12px 12px;
    margin: 8px;                   /* Space around the container */
    min-height: 56px;
}
```

### Example 4: Compact input bar (shorter, less padding)
```qss
QWidget#inputBarOuter {
    background: %50;
    border-bottom: 1px solid %51;
    padding: 6px 12px;             /* Was 12px - less vertical padding */
    min-height: 44px;              /* Was 56px - shorter */
}

QPushButton#btnInputMode,
QPushButton#btnInputModeArrow,
QLineEdit#txtMainInput,
QPushButton#btnApply {
    min-height: 28px;              /* Was 32px - shorter buttons */
    max-height: 28px;
}
```

### Example 5: Custom gradient background
```qss
QWidget#inputBarOuter {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #1a1a2e, 
                                stop:1 #2a2a3e);  /* Gradient from dark to lighter */
    border-bottom: 1px solid %51;
    padding: 12px 12px;
    min-height: 56px;
}
```

### Example 6: Spacious input bar (more padding on sides)
```qss
QWidget#inputBarOuter {
    padding: 12px 32px;            /* Was 12px 12px - more horizontal padding */
}

QLineEdit#txtMainInput {
    margin-left: 10px;             /* Was 6px */
}

QPushButton#btnApply {
    margin-left: 12px;             /* Was 8px */
}
```

### Example 7: Change button widths
```qss
QPushButton#btnInputMode {
    min-width: 100px;              /* Was 82px */
    max-width: 100px;
}

QPushButton#btnApply {
    min-width: 80px;               /* Was 64px */
    max-width: 80px;
}
```

## That's it!
No C++ code needed—just edit the `.qss` file to customize the input bar completely!

---

## 🎨 Quick Reference Cheat Sheet

### Most Common Container Tweaks

**Want a taller bar?**
```qss
QWidget#inputBarOuter {
    min-height: 70px;      /* Increase this */
    padding: 16px 12px;    /* Increase padding */
}
```

**Want a different background color?**
```qss
QWidget#inputBarOuter {
    background: #2a2a3e;   /* Change this to any color */
}
```

**Want more/less padding (space inside)?**
```qss
QWidget#inputBarOuter {
    padding: 20px 12px;    /* First number = top/bottom, second = left/right */
}
```

**Want rounded corners?**
```qss
QWidget#inputBarOuter {
    border-radius: 8px;    /* 0 = square, higher = rounder */
}
```

**Want a top border too?**
```qss
QWidget#inputBarOuter {
    border-top: 1px solid #444;
    border-bottom: 1px solid %51;
}
```

**Want full border (all sides)?**
```qss
QWidget#inputBarOuter {
    border: 1px solid #444;
}
```

**Want a different border color?**
```qss
QWidget#inputBarOuter {
    border-bottom: 2px solid #2db570;  /* Thickness, style, color */
}
```

**Want margin (space outside)?**
```qss
QWidget#inputBarOuter {
    margin: 10px;          /* Space around the entire container */
}
```
