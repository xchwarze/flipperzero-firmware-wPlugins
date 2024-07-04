## Main changes
- NFC: 
    - OFW: Ultralight C authentication with des key
    - EMV Transactions less nested, hide if unavailable (by @Willy-JL | PR #771)
* LF RFID: Update T5577 password list (by @korden32 | PR #774)
* JS: Refactor widget and keyboard modules, fix crash (by @Willy-JL | PR #770)
* OFW: Event Loop Timers
* OFW: Updater: resource compression
* Apps: **Check out more Apps updates and fixes by following** [this link](https://github.com/xMasterX/all-the-plugins/commits/dev)
## Other changes
* OFW: copro: bumped to 1.20.0
* OFW: input_srv: Put input state data on the stack of the service
* OFW: Coalesce some allocations
* OFW: Fix iButton/LFRFID Add Manually results being discarded
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
