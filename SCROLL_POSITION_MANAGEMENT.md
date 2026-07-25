# Scroll Position Management Implementation

## Overview
Implemented separate scroll position tracking for terminal and table views, with intelligent reset on first switch after a solve.

## Key Features

### 1. Separate Scroll Position Tracking
- **terminalScrollPositionRef**: Remembers terminal view scroll position
- **tableScrollPositionRef**: Remembers table view scroll position
- **firstTableSwitchAfterSolveRef**: Flag to detect first switch after solve

### 2. Smart Reset Behavior
When switching to table mode for the **first time** after a solve completes:
- Terminal scroll position → reset to 0 (top)
- Table scroll position → reset to 0 (top)
- Flag is set to false, so subsequent switches remember positions

### 3. Position Restoration
When switching between views:
- Current view's scroll position is saved
- Target view's saved scroll position is restored
- Uses `requestAnimationFrame` to restore after DOM renders

## Implementation Details

### New Functions

**switchToTableMode()**
```typescript
- Saves current terminal scroll position
- Checks if it's the first switch after solve
  - If first: Reset both positions to 0
  - If not: Keep saved positions
- Switches to table view
- Restores table's saved scroll position
```

**switchToTerminalMode()**
```typescript
- Saves current table scroll position
- Switches to terminal view
- Restores terminal's saved scroll position
```

**handleTableScroll()**
```typescript
- New scroll handler for table container
- Saves current table scroll position on every scroll
```

**handleTerminalScroll()** (enhanced)
```typescript
- Now also saves terminal scroll position
- Previously only tracked auto-scroll state
```

### Modified Sections

1. **State Initialization** (line 991-993)
   - Added 3 new refs for scroll tracking

2. **Scroll Handlers** (line 1061-1106)
   - Enhanced handleTerminalScroll()
   - Added handleTableScroll()
   - Added switchToTableMode()
   - Added switchToTerminalMode()

3. **Solve Function** (line 1359-1361)
   - Reset flags and scroll positions at solve start
   - Ensures fresh scroll state for new solve

4. **View Switching** (line 1395)
   - Use switchToTableMode() when auto-switching on solve complete

5. **Button Handlers** (line 1660, 1664)
   - Main toggle button: calls switchToTableMode() or switchToTerminalMode()
   - Completed-while-paused button: calls switchToTableMode()

6. **Table Container** (line 1667)
   - Added ref={tableContainerRef}
   - Added onScroll={handleTableScroll}

## Flow Diagram

```
User starts solve
    ↓
firstTableSwitchAfterSolveRef = true
terminalScrollPositionRef = 0
tableScrollPositionRef = 0
    ↓
User scrolls terminal → saved to terminalScrollPositionRef
    ↓
Auto-switch to table (first time)
    ↓
Check firstTableSwitchAfterSolveRef?
    ↓ (yes, first time)
Reset both positions to 0
Set flag to false
    ↓
Table renders at top
User scrolls table → saved to tableScrollPositionRef
    ↓
User clicks terminal button
    ↓
Table position → tableScrollPositionRef
Switch to terminal
Terminal renders at saved position
    ↓
User clicks table button (2nd time)
    ↓
Terminal position → terminalScrollPositionRef
Switch to table
    ↓
Check firstTableSwitchAfterSolveRef?
    ↓ (no, already switched once)
Use saved tableScrollPositionRef position
    ↓
Table renders at remembered scroll position
```

## Testing Instructions

1. **Basic Test**: First Switch After Solve
   - Start a solve
   - Manually scroll the terminal
   - Wait for auto-switch to table
   - ✓ Table should be at top (scrollTop = 0)

2. **Position Memory Test**: Subsequent Switches
   - Scroll the table down (e.g., to row 20)
   - Click terminal button → go to terminal
   - Scroll terminal up a bit
   - Click table button → go to table
   - ✓ Table should be at row 20 (saved position)

3. **Terminal Memory Test**:
   - Click terminal button
   - Scroll to specific position
   - Click table button
   - Click terminal button again
   - ✓ Terminal should be at saved position

4. **New Solve Reset Test**:
   - Complete a solve and switch to table
   - Scroll table to specific position
   - Start a new solve
   - Wait for auto-switch
   - ✓ Table should be at top again

## Files Modified

- **main/src/App.tsx**
  - Added 3 refs for scroll tracking
  - Added 2 functions for view switching
  - Enhanced scroll handlers
  - Updated button handlers
  - Modified solve function
  - Added ref and onScroll to table container

## Browser Behavior

The implementation uses standard DOM scrolling:
- `scrollTop` property for reading/writing scroll position
- `onScroll` event for detecting scroll changes
- `requestAnimationFrame` for smooth DOM updates

No external libraries required - uses native browser APIs.

## Edge Cases Handled

1. **Null refs**: Checks for element existence before accessing scrollTop
2. **First solve**: Flag properly initialized to true
3. **Multiple solves**: Flag reset on each new solve
4. **Rapid switching**: RAF ensures DOM is ready before restoring position
5. **Auto-scroll during solve**: Terminal auto-scroll still works independently

## Performance Considerations

- Minimal overhead: Just tracking numbers and flags
- RAF prevents layout thrashing
- No polling or timers
- Event-driven design (scroll events)

## Future Enhancements

Possible improvements (not implemented):
1. Persist scroll positions to localStorage
2. Restore positions on page reload
3. Smooth scroll animation instead of instant jump
4. Consider viewport height changes
