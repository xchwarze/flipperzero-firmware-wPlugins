## Main changes
- SubGHz:
    - **Novoferm** remotes full support
    - Fix Decode scene in RAW files
    - Add manually -> Add Sommer FM238 option for cases when default option doesn't work (named as Sommer fm2)
    - Remove broken preset modulation
    - Normstahl, Sommer, MHouse, Aprimatic -> Fixes for button codes and more in Add manually
    - Custom button improvements for MHouse, Novoferm, Nice Smilo
    - Hormann EcoStar -> Add manually support, and custom button support
    - Hormann HSM 44bit static -> Button code decoding fix
    - Choose RSSI threshold for Hopping mode (by @Willy-JL)
- NFC: 
    - OFW: Ultralight C authentication with des key
    - EMV Transactions less nested, hide if unavailable (by @Willy-JL | PR #771)
    - Update Mifare Classic default keys dict with new keys from proxmark3 repo and UberGuidoZ repo
- LF RFID: 
    - Update T5577 password list (by @korden32 | PR #774)
    - Add DEZ 8 display form for EM4100 (by @korden32 | PR #776 & (#777 by @mishamyte))
- JS: 
    - Refactor widget and keyboard modules, fix crash (by @Willy-JL | PR #770)
    - SubGHz module fixes and improvements (by @Willy-JL)
* OFW: Infrared: check for negative timings
* OFW: Fix iButton/LFRFID Add Manually results being discarded
* OFW: Event Loop Timers
* OFW: Updater: resource compression
* Apps: **Check out more Apps updates and fixes by following** [this link](https://github.com/xMasterX/all-the-plugins/commits/dev)
## Other changes
* OFW: Disabled ISR runtime stats collection for updater builds
* OFW: VSCode fixes: .gitignore & clangd
* OFW: ufbt: synced .clang-format rules with main
* OFW: Code formatting update 
* OFW: scripts: runfap: fixed starting apps with spaces in path
* OFW: toolchain: v38. clangd as default language server
* OFW: NFC: ISO15693 Render Typo Fix
* OFW: tar archive: fix double free
* OFW: ufbt: added ARGS to commandline parser
* OFW: lib: sconscript todo cleanup
* OFW: Intruder animation
* OFW: Desktop: allow to close blocking bad sd animation
* OFW: Updater: reset various debug flags on production build flash (was done in same way in UL before)
* OFW: Fix PVS Warnings
* OFW: CCID: Improve request and response data handling
* OFW: Furi: count ISR time. Cli: show ISR time in top.
* OFW: toolchain: v37
* OFW: NFC: Cache plugin name not full path, saves some RAM (by @Willy-JL)
* OFW: copro: bumped to 1.20.0
* OFW: input_srv: Put input state data on the stack of the service
* OFW: Coalesce some allocations
* OFW: updater: slightly smaller image
* OFW: Updater: Fix double dir cleanup
* OFW: cli: storage: minor subcommand lookup refactor
* OFW: LFRFID Securakey: Add Support for RKKTH Plain Text Format
* OFW: NFC: Add mf_classic_set_sector_trailer_read function
* OFW: Separate editing and renaming in iButton and LFRFID
* OFW: New js modules documentation added 
* OFW: Update link to mfkey32
* OFW: NFC: Desfire Renderer Minor Debug 
* OFW: RPC: Fix input lockup on disconnect
* OFW: Thread Signals
<br><br>
#### Known NFC post-refactor regressions list: 
- Mifare Mini clones reading is broken (original mini working fine) (OFW)
- NFC CLI was removed with refactoring (OFW) (will be back soon)

### THANKS TO ALL RM SPONSORS FOR BEING AWESOME!

# MOST OF ALL, THANK YOU TO THE FLIPPER ZERO COMMUNITY THAT KEEPS GROWING OUR PROJECT!
