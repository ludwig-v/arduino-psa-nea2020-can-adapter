
# arduino-psa-nea2020-can-adapter
Arduino sketch to operate new PSA devices (NEA2020) - especially IVI - on AEE2010 BSI

### Work in progress

The current sketch is a PoC (Proof of Concept), still have many things to fix:

 - Map ambiance colors on 0x220
 - Fix AAS sensors
 - Fix CVM / Speed sign issue
 - Fix Cruise control

### Real life examples
> ![IVI on 208](https://i.imgur.com/Rn7CdSc.jpg)
  IVI on 208 (P21E)

### IVI Zone changes

Fix 2cm black headband on HangSheng 10" by changing screen type from **C2** to **C**

| Zone | Data byte[0] | Data byte[1] | Data byte[2] | Data byte[3] | Data byte[4] |
|----|--|--|--|--|--|
|  2102  | 04 | 11 | 10 | 0A | 0**8** |
|  | | | | | **DGT10C2** |
|  2102  | 04 | 11 | 10 | 0A | 0**7** |
|  | | | | | **DGT10C** |

### Special thanks

🇮🇹 [Hallahub](https://www.forum-peugeot.com/Forum/members/hallahub.114118/) from Forum-Peugeot
