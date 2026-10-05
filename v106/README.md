# Replay-derived bot, version 4

Same Python submission format as the supplied bot; main.py is the entry point and bot.toml is unchanged.

Changes from v3: recognise both the standard 326-bed Schooltime map and the recovered 178-bed variant. On Trauma only, long dragons can reuse the existing food search's scored target path for a guarded two-step free transit. Visibility, body, enemy, exit and queen checks remain. Replay seeds and recorded actions are not used at runtime.

Selected-source comparisons on seed 227: 5/6 wins against v3 on Trauma and both Schooltime layouts; 2/2 against the supplied original on Trauma. Zero Python exceptions. This selected map sample does not establish an overall tournament improvement or target parity.

Official metered sandbox: 4122 turns across 16 lifetimes, no errors or output differences; peak 76,139,744/100,000,000 points and 22,740,992 bytes. Budgets in every possible state remain unverified.

Extract and submit as with the original bot. No upload was performed. See accompanying analysis-v4.md, test_results-v4.csv and verification-v4.json for details.
