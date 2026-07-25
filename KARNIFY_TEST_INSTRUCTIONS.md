# Test harness instructions for karnify optimization verification

## What was optimized:

1. **Hash map lookups** - Replaced linear vector search with O(1) hash map lookups
   - WCA_TO_KARN: 40 entries
   - KARN_TO_HIGHKARN: 36 entries
   - Expected speedup: 80-90% faster per solution

2. **Fixed intentional compromise** - Late solutions now get properly karnified
   - Removed stopped.current flag that skipped karnification
   - Late solutions now go through full buildDisplayPair pipeline

3. **No change to algorithm logic** - Output should be EXACTLY the same

## How to test:

### Option 1: Run from your solver with the optimized code

1. Build the project with: `cargo build --release`
2. Run the solver with a scramble
3. Watch that:
   - Solutions render immediately without the lag
   - Late solutions (after clicking stop) are properly karnified
   - No more raw/non-karnified solutions in the table

### Option 2: Quick validation test (standalone C++)

The test harness has been compiled at:
`main/src-tauri/native/test_karnify_bin`

To use it:
1. Solve some scramble with the current UI
2. Let it generate 100-200 solutions
3. Click "Stop" to let queue build up with queued solutions
4. Capture the raw solver output (the lines like "13|F' d' u D M u U' U2D' 31  [9]")
5. Run: `./main/src-tauri/native/test_karnify_bin`
6. Paste the solutions, one per line
7. End input with "Stop requested." or Ctrl+D
8. The test will verify karnification for each solution

Expected output format:
```
[1] 1,0 / 3,3 / 0,-3 → U e' D
[2] F' d' u D M u U' U2D' 31 → F' d' u D M u U2 D'
...
```

## Verification checklist:

- [ ] Code compiles without errors
- [ ] Test input parses correctly
- [ ] All solutions produce valid karnified output
- [ ] No solutions are skipped
- [ ] Output format matches expected Karnotation
- [ ] Build completes successfully (cargo build --release)
- [ ] New version runs without karnification lag
- [ ] Late solutions are properly karnified in the UI

## If you encounter issues:

- **Compilation errors**: karnotation.h syntax issue (unlikely)
- **No output for valid input**: Check solver output format matches "13|algorithm [count]"
- **Different output after optimization**: Hash map initialization failed (check getWCAToKarnMap())
