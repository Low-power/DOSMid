/*
 * This file contains OPL fonts for all the General Midi level 1 instruments:
 *  - 128 instruments
 *  - 52 percussions
 *
 * Percussions follow immediately the instruments for space efficiency. But
 * GM1 maps them on range 35..86. Hence to lookup a percussion here, one has
 * to look at gmtimbres[128 + GM_PERCUSSION_KEY - 35] (take care never to look
 * up any percussion above 86 or below 35).
 *
 * These patches are pulled from the ADLMIDI (GPL) project, that collected
 * them itself from all around. Joel Yliluoma (aka bisqwit), ADLMIDI's author,
 * writes:
 * > The FM soundfonts (patches) used in the program are imported from various
 * > PC games without permission from their respective authors. The question
 * > of copyright, when it comes to sets of 11 numeric bytes, is somewhat
 * > vague, especially considering that most of those sets are simply
 * > descendants of the patch sets originally published by AdLib Inc. for
 * > everyone's free use.
 */

static struct timbre_t gmtimbres[256] = {
  /*    ,---------+-------- Wave select settings                           *
   *    | ,-------÷-+------ Sustain/release rates                          *
   *    | | ,-----÷-÷-+---- Attack/decay rates                             *
   *    | | | ,---÷-÷-÷-+-- AM/VIB/EG/KSR/Multiple bits                    *
   *    | | | |   | | | |                                                  *
   *    | | | |   | | | |     ,----+-- KSL/attenuation settings            *
   *    | | | |   | | | |     |    |    ,----- Feedback/connection bits    *
   *    | | | |   | | | |     |    |    |                                  */
    { 0x0F4F201,0x0F7F201, 0x8F,0x06, 0x8,+0 }, /* 0: BisqM0; GM0; b13M0; f29GM0; f30GM0; fat2M0; sGM0; AcouGrandPiano; am000 */
    { 0x0F4F201,0x0F7F201, 0x4B,0x00, 0x8,+0 }, /* 1: GM1; b13M1; f29GM1; f30GM1; fat2M1; sGM1; BrightAcouGrand; am001 */
    { 0x0F4F201,0x0F6F201, 0x49,0x00, 0x8,+0 }, /* 2: BisqM2; GM2; b13M2; f29GM2; f30GM2; f34GM0; f34GM1; f34GM2; fat2M2; sGM2; AcouGrandPiano; BrightAcouGrand; ElecGrandPiano; am002 */
    { 0x0F7F281,0x0F7F241, 0x12,0x00, 0x6,+0 }, /* 3: GM3; b13M3; f34GM3; fat2M3; sGM3; Honky-tonkPiano; am003 */
    { 0x0F7F101,0x0F7F201, 0x57,0x00, 0x0,+0 }, /* 4: GM4; b13M4; f34GM4; fat2M4; sGM4; Rhodes Piano; am004 */
    { 0x0F7F101,0x0F7F201, 0x93,0x00, 0x0,+0 }, /* 5: GM5; b13M5; f29GM6; f30GM6; f34GM5; fat2M5; sGM5; Chorused Piano; Harpsichord; am005 */
    { 0x0F2A101,0x0F5F216, 0x80,0x0E, 0x8,+0 }, /* 6: GM6; b13M6; f34GM6; fat2M6; Harpsichord; am006 */
    { 0x0F8C201,0x0F8C201, 0x92,0x00, 0xA,+0 }, /* 7: GM7; b13M7; f34GM7; fat2M7; sGM7; Clavinet; am007 */
    { 0x0F4F60C,0x0F5F381, 0x5C,0x00, 0x0,+0 }, /* 8: GM8; b13M8; f34GM8; fat2M8; sGM8; Celesta; am008 */
    { 0x0F2F307,0x0F1F211, 0x97,0x80, 0x2,+0 }, /* 9: BisqM9; GM9; b13M9; f29GM101; f30GM101; f34GM9; fat2M9; FX 6 goblins; Glockenspiel; am009 */
    { 0x0F45417,0x0F4F401, 0x21,0x00, 0x2,+0 }, /* 10: GM10; b13M10; f29GM100; f30GM100; f34GM10; fat2M10; sGM10; FX 5 brightness; Music box; am010 */
    { 0x0F6F398,0x0F6F281, 0x62,0x00, 0x0,+0 }, /* 11: BisqM11; GM11; b13M11; f34GM11; fat2M11; sGM11; Vibraphone; am011 */
    { 0x0F6F618,0x0F7E701, 0x23,0x00, 0x0,+0 }, /* 12: GM12; b13M12; f29GM104; f29GM97; f30GM104; f30GM97; f34GM12; fat2M12; sGM12; FX 2 soundtrack; Marimba; Sitar; am012 */
    { 0x0F6F615,0x0F6F601, 0x91,0x00, 0x4,+0 }, /* 13: GM13; b13M13; f29GM103; f30GM103; f34GM13; fat2M13; sGM13; FX 8 sci-fi; Xylophone; am013 */
    { 0x0F3D345,0x0F3A381, 0x59,0x80, 0xC,+0 }, /* 14: GM14; b13M14; f34GM14; fat2M14; Tubular Bells; am014 */
    { 0x1F57503,0x0F5B581, 0x49,0x80, 0x4,+0 }, /* 15: GM15; b13M15; f34GM15; fat2M15; sGM15; Dulcimer; am015 */
    { 0x014F671,0x007F131, 0x92,0x00, 0x2,+0 }, /* 16: GM16; HMIGM16; b13M16; b7M16; f34GM16; fat2M16; sGM16; Hammond Organ; am016; am016.in */
    { 0x058C772,0x008C730, 0x14,0x00, 0x2,+0 }, /* 17: GM17; HMIGM17; b13M17; b7M17; f34GM17; fat2M17; sGM17; Percussive Organ; am017; am017.in */
    { 0x018AA70,0x0088AB1, 0x44,0x00, 0x4,+0 }, /* 18: GM18; HMIGM18; b13M18; b7M18; f34GM18; fat2M18; sGM18; Rock Organ; am018; am018.in */
    { 0x1239723,0x01455B1, 0x93,0x00, 0x4,+0 }, /* 19: GM19; HMIGM19; b13M19; b7M19; f34GM19; fat2M19; Church Organ; am019; am019.in */
    { 0x1049761,0x00455B1, 0x13,0x80, 0x0,+0 }, /* 20: BisqM20; GM20; HMIGM20; b13M20; b7M20; f34GM20; fat2M20; sGM20; Reed Organ; am020; am020.in */
    { 0x12A9824,0x01A46B1, 0x48,0x00, 0xC,+0 }, /* 21: GM21; HMIGM21; b13M21; b7M21; f34GM21; fat2M21; sGM21; Accordion; am021; am021.in */
    { 0x1069161,0x0076121, 0x13,0x00, 0xA,+0 }, /* 22: GM22; HMIGM22; b13M22; b7M22; f34GM22; fat2M22; sGM22; Harmonica; am022; am022.in */
    { 0x0067121,0x00761A1, 0x13,0x89, 0x6,+0 }, /* 23: GM23; HMIGM23; b13M23; b7M23; f34GM23; fat2M23; sGM23; Tango Accordion; am023; am023.in */
    { 0x194F302,0x0C8F341, 0x9C,0x80, 0xC,+0 }, /* 24: GM24; HMIGM24; b13M24; b7M24; f34GM24; fat2M24; Acoustic Guitar1; am024; am024.in */
    { 0x19AF303,0x0E7F111, 0x54,0x00, 0xC,+0 }, /* 25: GM25; HMIGM25; b13M25; b7M25; f17GM25; f29GM60; f30GM60; f34GM25; fat2M25; mGM25; sGM25; Acoustic Guitar2; French Horn; am025; am025.in */
    { 0x03AF123,0x0F8F221, 0x5F,0x00, 0x0,+0 }, /* 26: GM26; HMIGM26; b13M26; b7M26; b8M26; f17GM26; f34GM26; f35GM26; fat2M26; mGM26; sGM26; Electric Guitar1; am026; am026.in; jazzgtr */
    { 0x122F603,0x0F8F321, 0x87,0x80, 0x6,+0 }, /* 27: GM27; b13M27; f30GM61; f34GM27; fat2M27; sGM27; Brass Section; Electric Guitar2; am027 */
    { 0x054F903,0x03AF621, 0x47,0x00, 0x0,+0 }, /* 28: GM28; HMIGM28; b13M28; b6M107; b6M3; b6M99; b7M28; b8M20; b8M28; f17GM28; f34GM28; f35GM28; fat2M28; hamM3; hamM60; intM3; mGM28; rickM3; sGM28; BPerc; BPerc.in; Electric Guitar3; RBPerc; Rmutegit; am028; am028.in; muteguit */
    { 0x1419123,0x0198421, 0x4A,0x05, 0x8,+0 }, /* 29: GM29; b13M29; f34GM29; fat2M29; sGM29; Overdrive Guitar; am029 */
    { 0x1199523,0x0199421, 0x4A,0x00, 0x8,+0 }, /* 30: GM30; HMIGM30; b13M30; b6M116; b6M6; b7M30; f17GM30; f34GM30; f35GM30; fat2M30; hamM6; intM6; mGM30; rickM6; sGM30; Distorton Guitar; GDist; GDist.in; RGDist; am030; am030.in */
    { 0x04F2009,0x0F8D184, 0xA1,0x80, 0x8,+0 }, /* 31: GM31; HMIGM31; b13M31; b6M104; b6M5; b7M31; b8M120; f34GM31; fat2M31; hamM5; intM5; rickM5; sGM31; Feedbck; GFeedbck; Guitar Harmonics; RFeedbck; am031; am031.in */
    { 0x0069421,0x0A6C3A2, 0x1E,0x00, 0x2,+0 }, /* 32: GM32; HMIGM32; b13M32; b7M32; f34GM32; fat2M32; sGM32; Acoustic Bass; am032; am032.in */
    { 0x028F131,0x018F131, 0x12,0x00, 0xA,+0 }, /* 33: GM33; GM39; HMIGM33; HMIGM39; b13M33; b13M39; b7M33; b7M39; f15GM30; f17GM33; f17GM39; f26GM30; f29GM28; f29GM29; f30GM28; f30GM29; f34GM33; f34GM39; f35GM39; fat2M33; fat2M39; hamM68; mGM33; mGM39; sGM33; sGM39; Distorton Guitar; Electric Bass 1; Electric Guitar3; Overdrive Guitar; Synth Bass 2; am033; am033.in; am039; am039.in; synbass2 */
    { 0x0E8F131,0x078F131, 0x8D,0x00, 0xA,+0 }, /* 34: BisqM37; GM34; HMIGM34; b13M34; b7M34; b8M37; f15GM28; f17GM34; f26GM28; f29GM38; f29GM67; f30GM38; f30GM67; f34GM34; f35GM37; fat2M34; mGM34; rickM81; sGM34; Baritone Sax; Electric Bass 2; Electric Guitar3; Slap Bass 2; Slapbass; Synth Bass 1; am034; am034.in; slapbass */
    { 0x0285131,0x0487132, 0x5B,0x00, 0xC,+0 }, /* 35: BisqM35; GM35; HMIGM35; b13M35; b50M35; b51M35; b7M35; f17GM35; f20GM35; f29GM42; f29GM70; f29GM71; f30GM42; f30GM70; f30GM71; f31GM35; f34GM35; f36GM35; f49GM35; fat2M35; mGM35; qGM35; sGM35; Bassoon; Cello; Clarinet; Fretless Bass; am035; am035.in; gm035 */
    { 0x09AA101,0x0DFF221, 0x8B,0x40, 0x8,+0 }, /* 36: GM36; HMIGM36; b13M36; b50M36; b51M36; b7M36; f17GM36; f20GM36; f29GM68; f30GM68; f31GM36; f34GM36; f36GM36; f49GM36; fat2M36; mGM36; qGM36; sGM36; Oboe; Slap Bass 1; am036; am036.in; gm036 */
    { 0x016A221,0x0DFA121, 0x8B,0x08, 0x8,+0 }, /* 37: GM37; b13M37; f29GM69; f30GM69; f34GM37; fat2M37; sGM37; English Horn; Slap Bass 2; am037 */
    { 0x0E8F431,0x078F131, 0x8B,0x00, 0xA,+0 }, /* 38: GM38; HMIGM38; b13M38; b6M121; b6M13; b7M38; b8M84; f17GM38; f29GM30; f29GM31; f30GM30; f30GM31; f34GM38; f35GM38; fat2M38; hamM13; hamM67; intM13; mGM38; rickM13; sGM38; BSynth3; BSynth3.; Distorton Guitar; Guitar Harmonics; RBSynth3; Synth Bass 1; am038; am038.in; synbass1 */
    { 0x028F131,0x018F131, 0x12,0x00, 0xA,+0 }, /* 39: GM33; GM39; HMIGM33; HMIGM39; b13M33; b13M39; b7M33; b7M39; f15GM30; f17GM33; f17GM39; f26GM30; f29GM28; f29GM29; f30GM28; f30GM29; f34GM33; f34GM39; f35GM39; fat2M33; fat2M39; hamM68; mGM33; mGM39; sGM33; sGM39; Distorton Guitar; Electric Bass 1; Electric Guitar3; Overdrive Guitar; Synth Bass 2; am033; am033.in; am039; am039.in; synbass2 */
    { 0x113DD31,0x0265621, 0x15,0x00, 0x8,+0 }, /* 40: GM40; HMIGM40; b13M40; b50M40; b51M40; b7M40; f17GM40; f20GM40; f31GM40; f34GM40; f36GM40; f48GM40; f49GM40; fat2M40; mGM40; qGM40; sGM40; Violin; am040; am040.in; gm040 */
    { 0x113DD31,0x0066621, 0x16,0x00, 0x8,+0 }, /* 41: GM41; HMIGM41; b13M41; b7M41; f17GM41; f34GM41; fat2M41; mGM41; sGM41; Viola; am041; am041.in */
    { 0x11CD171,0x00C6131, 0x49,0x00, 0x8,+0 }, /* 42: GM42; HMIGM42; b13M42; b7M42; f34GM42; fat2M42; sGM42; Cello; am042; am042.in */
    { 0x1127121,0x0067223, 0x4D,0x80, 0x2,+0 }, /* 43: GM43; HMIGM43; b13M43; b7M43; b8M43; f17GM43; f29GM56; f30GM56; f34GM43; f35GM43; fat2M43; mGM43; sGM43; Contrabass; Trumpet; am043; am043.in; contrbs1 */
    { 0x121F1F1,0x0166FE1, 0x40,0x00, 0x2,+0 }, /* 44: GM44; HMIGM44; b13M44; b7M44; b8M44; f17GM44; f34GM44; f35GM44; fat2M44; mGM44; Tremulo Strings; am044; am044.in; tremstr */
    { 0x175F502,0x0358501, 0x1A,0x80, 0x0,+0 }, /* 45: GM45; HMIGM45; b13M45; b7M45; f17GM45; f29GM51; f30GM51; f34GM45; fat2M45; mGM45; Pizzicato String; SynthStrings 2; am045; am045.in */
    { 0x175F502,0x0F4F301, 0x1D,0x80, 0x0,+0 }, /* 46: GM46; HMIGM46; b13M46; b7M46; f15GM57; f15GM58; f17GM46; f26GM57; f26GM58; f29GM57; f29GM58; f30GM57; f30GM58; f34GM46; fat2M46; mGM46; oGM57; oGM58; Orchestral Harp; Trombone; Tuba; am046; am046.in */
    { 0x105F510,0x0C3F211, 0x41,0x00, 0x2,+0 }, /* 47: BisqM47; GM47; HMIGM47; b13M47; b50M47; b51M47; b6M14; b7M47; b8M86; f17GM47; f20GM47; f30GM112; f34GM47; f36GM47; fat2M47; hamM14; intM14; mGM47; qGM47; rickM14; BSynth4; BSynth4.; Timpany; Tinkle Bell; am047; am047.in; gm047 */
    { 0x125B121,0x00872A2, 0x9B,0x01, 0xE,+0 }, /* 48: GM48; HMIGM48; b13M48; b7M48; f34GM48; fat2M48; String Ensemble1; am048; am048.in */
    { 0x1037FA1,0x1073F21, 0x98,0x00, 0x0,+0 }, /* 49: GM49; HMIGM49; b13M49; b7M49; f34GM49; fat2M49; String Ensemble2; am049; am049.in */
    { 0x012C1A1,0x0054F61, 0x93,0x00, 0xA,+0 }, /* 50: GM50; HMIGM50; b13M50; b6M20; b7M50; f34GM50; fat2M50; hamM20; intM20; rickM20; PMellow; PMellow.; Synth Strings 1; am050; am050.in */
    { 0x022C121,0x0054F61, 0x18,0x00, 0xC,+0 }, /* 51: BisqM51; GM51; HMIGM51; b13M51; b50M51; b51M51; b7M51; f20GM51; f31GM51; f34GM51; f36GM51; f48GM51; f49GM51; fat2M51; qGM51; sGM51; SynthStrings 2; am051; am051.in; gm051 */
    { 0x015F431,0x0058A72, 0x5B,0x83, 0x0,+0 }, /* 52: GM52; HMIGM52; b13M52; b6M125; b7M52; b8M52; f34GM52; fat2M52; rickM85; Choir Aahs; Choir.in; RChoir; am052; am052.in; choir */
    { 0x03974A1,0x0677161, 0x90,0x00, 0x0,+0 }, /* 53: GM53; HMIGM53; b13M53; b7M53; b8M53; f34GM53; fat2M53; rickM86; sGM53; Oohs.ins; Voice Oohs; am053; am053.in; oohs */
    { 0x0055471,0x0057A72, 0x57,0x00, 0xC,+0 }, /* 54: BisqM54; GM54; HMIGM54; b13M54; b7M54; b8M54; f34GM54; fat2M54; sGM54; Synth Voice; am054; am054.in; synvox2 */
    { 0x0635490,0x045A541, 0x00,0x00, 0x8,+0 }, /* 55: GM55; HMIGM55; b13M55; b7M55; f34GM55; fat2M55; Orchestra Hit; am055; am055.in */
    { 0x0178521,0x0098F21, 0x92,0x01, 0xC,+0 }, /* 56: BisqM56; GM56; HMIGM56; b13M56; b50M56; b51M56; b7M56; f17GM56; f20GM56; f31GM56; f34GM56; f36GM56; f49GM56; fat2M56; mGM56; qGM56; Trumpet; am056; am056.in; gm056 */
    { 0x0177521,0x0098F21, 0x94,0x05, 0xC,+0 }, /* 57: BisqM57; GM57; HMIGM57; b13M57; b7M57; f17GM57; f29GM90; f30GM90; f34GM57; fat2M57; mGM57; Pad 3 polysynth; Trombone; am057; am057.in */
    { 0x0157621,0x0378261, 0x94,0x00, 0xC,+0 }, /* 58: GM58; HMIGM58; b13M58; b7M58; f34GM58; fat2M58; Tuba; am058; am058.in */
    { 0x1179E31,0x12C6221, 0x43,0x00, 0x2,+0 }, /* 59: GM59; HMIGM59; b13M59; b7M59; f17GM59; f34GM59; f35GM59; fat2M59; mGM59; sGM59; Muted Trumpet; am059; am059.in */
    { 0x06A6121,0x00A7F21, 0x9B,0x00, 0x2,+0 }, /* 60: GM60; HMIGM60; b13M60; b7M60; f17GM60; f29GM92; f29GM93; f30GM92; f30GM93; f34GM60; f48GM62; fat2M60; mGM60; French Horn; Pad 5 bowedpad; Pad 6 metallic; Synth Brass 1; am060; am060.in */
    { 0x01F7561,0x00F7422, 0x8A,0x06, 0x8,+0 }, /* 61: GM61; HMIGM61; b13M61; b7M61; f34GM61; fat2M61; Brass Section; am061; am061.in */
    { 0x15572A1,0x0187121, 0x86,0x83, 0x0,+0 }, /* 62: GM62; b13M62; f34GM62; fat2M62; sGM62; Synth Brass 1; am062 */
    { 0x03C5421,0x01CA621, 0x4D,0x00, 0x8,+0 }, /* 63: GM63; HMIGM63; b13M63; b7M63; f17GM63; f29GM26; f29GM44; f30GM26; f30GM44; f34GM63; fat2M63; mGM63; sGM63; Electric Guitar1; Synth Brass 2; Tremulo Strings; am063; am063.in */
    { 0x1029331,0x00B7261, 0x8F,0x00, 0x8,+0 }, /* 64: GM64; HMIGM64; b13M64; b7M64; f34GM64; fat2M64; sGM64; Soprano Sax; am064; am064.in */
    { 0x1039331,0x0097261, 0x8E,0x00, 0x8,+0 }, /* 65: GM65; HMIGM65; b13M65; b7M65; f34GM65; fat2M65; sGM65; Alto Sax; am065; am065.in */
    { 0x1039331,0x0098261, 0x91,0x00, 0xA,+0 }, /* 66: GM66; HMIGM66; b13M66; b7M66; f34GM66; fat2M66; sGM66; Tenor Sax; am066; am066.in */
    { 0x10F9331,0x00F7261, 0x8E,0x00, 0xA,+0 }, /* 67: GM67; HMIGM67; b13M67; b7M67; f34GM67; fat2M67; sGM67; Baritone Sax; am067; am067.in */
    { 0x116AA21,0x00A8F21, 0x4B,0x00, 0x8,+0 }, /* 68: GM68; HMIGM68; b13M68; b7M68; f17GM68; f29GM84; f30GM84; f34GM68; fat2M68; mGM68; Lead 5 charang; Oboe; am068; am068.in */
    { 0x1177E31,0x10C8B21, 0x90,0x00, 0x6,+0 }, /* 69: GM69; HMIGM69; b13M69; b7M69; b8M69; f17GM69; f29GM85; f30GM85; f34GM69; f35GM69; fat2M69; mGM69; sGM69; English Horn; Lead 6 voice; am069; am069.in; ehorn */
    { 0x1197531,0x0196132, 0x81,0x00, 0x0,+0 }, /* 70: GM70; HMIGM70; b13M70; b7M70; f17GM70; f29GM86; f30GM86; f34GM70; fat2M70; mGM70; Bassoon; Lead 7 fifths; am070; am070.in */
    { 0x0219B32,0x0177221, 0x90,0x00, 0x4,+0 }, /* 71: GM71; HMIGM71; b13M71; b7M71; f17GM71; f29GM82; f29GM83; f30GM82; f30GM83; f34GM71; fat2M71; mGM71; Clarinet; Lead 3 calliope; Lead 4 chiff; am071; am071.in */
    { 0x05F85E1,0x01A65E1, 0x1F,0x00, 0x0,+0 }, /* 72: BisqM72; GM72; HMIGM72; b13M72; b7M72; b8M72; f17GM72; f34GM72; f35GM72; fat2M72; mGM72; Piccolo; am072; am072.in; picco */
    { 0x05F88E1,0x01A65E1, 0x46,0x00, 0x0,+0 }, /* 73: BisqM73; GM73; HMIGM73; b13M73; b7M73; f17GM73; f29GM72; f29GM73; f30GM72; f30GM73; f34GM73; fat2M73; mGM73; Flute; Piccolo; am073; am073.in */
    { 0x01F75A1,0x00A7521, 0x9C,0x00, 0x2,+0 }, /* 74: GM74; HMIGM74; b13M74; b7M74; fat2M74; sGM74; Recorder; am074; am074.in */
    { 0x0588431,0x01A6521, 0x8B,0x00, 0x0,+0 }, /* 75: GM75; HMIGM75; b13M75; b7M75; b8M75; f17GM75; f29GM77; f30GM77; f34GM75; f35GM75; fat2M75; mGM75; sGM75; Pan Flute; Shakuhachi; am075; am075.in; panflut1 */
    { 0x05666E1,0x02665A1, 0x4C,0x00, 0x0,+0 }, /* 76: GM76; HMIGM76; b13M76; b7M76; b8M76; f34GM76; fat2M76; sGM76; Bottle Blow; am076; am076.in; bottblow */
    { 0x0467662,0x03655A1, 0xCB,0x00, 0x0,+0 }, /* 77: GM77; HMIGM77; b13M77; b7M77; f34GM77; fat2M77; sGM77; Shakuhachi; am077; am077.in */
    { 0x0075762,0x00756A1, 0x99,0x00, 0xB,+0 }, /* 78: GM78; HMIGM78; b13M78; b7M78; f34GM78; fat2M78; sGM78; Whistle; am078; am078.in */
    { 0x0077762,0x00776A1, 0x93,0x00, 0xB,+0 }, /* 79: GM79; HMIGM79; b13M79; b7M79; f34GM79; fat2M79; hamM61; sGM79; Ocarina; am079; am079.in; ocarina */
    { 0x203FF22,0x00FFF21, 0x59,0x00, 0x0,+0 }, /* 80: GM80; HMIGM80; b13M80; b6M16; b7M80; f17GM80; f29GM47; f30GM47; f34GM80; f35GM80; f47GM80; fat2M80; hamM16; hamM65; intM16; mGM80; rickM16; sGM80; LSquare; LSquare.; Lead 1 squareea; Timpany; am080; am080.in; squarewv */
    { 0x10FFF21,0x10FFF21, 0x0E,0x00, 0x0,+0 }, /* 81: GM81; HMIGM81; b13M81; b7M81; f17GM81; f34GM81; fat2M81; mGM81; sGM81; Lead 2 sawtooth; am081; am081.in */
    { 0x0558622,0x0186421, 0x46,0x80, 0x0,+0 }, /* 82: GM82; HMIGM82; b13M82; b7M82; f17GM82; f34GM82; fat2M82; mGM82; sGM82; Lead 3 calliope; am082; am082.in */
    { 0x0126621,0x00A96A1, 0x45,0x00, 0x0,+0 }, /* 83: BisqM83; GM83; HMIGM83; b13M83; b7M83; f34GM83; fat2M83; sGM83; Lead 4 chiff; am083; am083.in */
    { 0x12A9221,0x02A9122, 0x8B,0x00, 0x0,+0 }, /* 84: GM84; HMIGM84; b13M84; b7M84; f17GM84; f34GM84; fat2M84; mGM84; sGM84; Lead 5 charang; am084; am084.in */
    { 0x005DFA2,0x0076F61, 0x9E,0x40, 0x2,+0 }, /* 85: GM85; HMIGM85; b13M85; b6M17; b7M85; b8M85; f34GM85; fat2M85; hamM17; intM17; rickM17; rickM87; sGM85; Lead 6 voice; PFlutes; PFlutes.; Solovox.; am085; am085.in; solovox */
    { 0x001EF20,0x2068F60, 0x1A,0x00, 0x0,+0 }, /* 86: GM86; HMIGM86; b13M86; b7M86; b8M81; f34GM86; fat2M86; rickM93; sGM86; Lead 7 fifths; Saw_wave; am086; am086.in */
    { 0x029F121,0x009F421, 0x8F,0x80, 0xA,+0 }, /* 87: BisqM87; GM87; HMIGM87; b13M87; b50M87; b51M87; b7M87; b8M87; f17GM87; f20GM87; f31GM87; f34GM87; f35GM87; f36GM87; fat2M87; mGM87; qGM87; sGM87; Lead 8 brass; am087; am087.in; baslead; gm087 */
    { 0x0945377,0x005A0A1, 0xA5,0x00, 0x2,+0 }, /* 88: GM88; HMIGM88; b13M88; b7M88; f34GM88; fat2M88; sGM88; Pad 1 new age; am088; am088.in */
    { 0x011A861,0x00325B1, 0x1F,0x80, 0xA,+0 }, /* 89: BisqM89; GM89; HMIGM89; b13M89; b50M89; b51M89; b7M89; b8M89; f20GM89; f31GM89; f34GM89; f36GM89; f49GM89; fat2M89; qGM89; sGM89; Pad 2 warm; am089; am089.in; gm089; warmpad */
    { 0x0349161,0x0165561, 0x17,0x00, 0xC,+0 }, /* 90: GM90; HMIGM90; b13M90; b6M21; b7M90; f34GM90; fat2M90; hamM21; intM21; rickM21; sGM90; LTriang; LTriang.; Pad 3 polysynth; am090; am090.in */
    { 0x0015471,0x0036A72, 0x5D,0x00, 0x0,+0 }, /* 91: GM91; HMIGM91; b13M91; b6M97; b7M91; b8M91; f34GM91; fat2M91; rickM95; sGM91; Pad 4 choir; RSpacevo; Spacevo.; am091; am091.in; spacevo */
    { 0x0432121,0x03542A2, 0x97,0x00, 0x8,+0 }, /* 92: BisqM92; GM92; HMIGM92; b13M92; b7M92; b8M92; f34GM92; f47GM92; fat2M92; sGM92; Pad 5 bowedpad; am092; am092.in; bowedgls */
    { 0x177A1A1,0x1473121, 0x1C,0x00, 0x0,+0 }, /* 93: BisqM93; GM93; HMIGM93; b13M93; b6M22; b7M93; b8M93; f34GM93; fat2M93; hamM22; intM22; rickM22; sGM93; PSlow; PSlow.in; Pad 6 metallic; am093; am093.in; metalpad */
    { 0x0331121,0x0254261, 0x89,0x03, 0xA,+0 }, /* 94: GM94; HMIGM94; b13M94; b6M112; b6M23; b7M94; b8M94; f34GM94; fat2M94; hamM23; hamM54; intM23; rickM23; rickM96; sGM94; Halopad.; PSweep; PSweep.i; Pad 7 halo; RHalopad; am094; am094.in; halopad */
    { 0x14711A1,0x007CF21, 0x15,0x00, 0x0,+0 }, /* 95: GM95; HMIGM95; b13M95; b6M119; b7M95; b8M95; f34GM95; f47GM95; fat2M95; hamM66; rickM97; Pad 8 sweep; RSweepad; Sweepad.; am095; am095.in; sweepad */
    { 0x0F6F83A,0x0028651, 0xCE,0x00, 0x2,+0 }, /* 96: GM96; HMIGM96; b13M96; b7M96; f34GM96; fat2M96; sGM96; FX 1 rain; am096; am096.in */
    { 0x1232121,0x0134121, 0x15,0x00, 0x0,+0 }, /* 97: GM97; HMIGM97; b13M97; b7M97; f17GM97; f29GM36; f30GM36; f34GM97; fat2M97; mGM97; sGM97; FX 2 soundtrack; Slap Bass 1; am097; am097.in */
    { 0x0957406,0x072A501, 0x5B,0x00, 0x0,+0 }, /* 98: GM98; HMIGM98; b13M98; b7M98; b8M98; f17GM98; f34GM98; f35GM98; fat2M98; mGM98; sGM98; FX 3 crystal; am098; am098.in; crystal1 */
    { 0x081B122,0x026F261, 0x92,0x83, 0xC,+0 }, /* 99: BisqM99; GM99; HMIGM99; b13M99; b7M99; f34GM99; fat2M99; sGM99; FX 4 atmosphere; am099; am099.in */
    { 0x151F141,0x0F5F242, 0x4D,0x00, 0x0,+0 }, /* 100: BisqM100; GM100; HMIGM100; b13M100; b7M100; b8M100; f34GM100; fat2M100; hamM51; sGM100; FX 5 brightness; am100; am100.in; bright */
    { 0x1511161,0x01311A3, 0x94,0x80, 0x6,+0 }, /* 101: GM101; HMIGM101; b13M101; b6M118; b7M101; b8M101; f34GM101; fat2M101; sGM101; FX 6 goblins; Rgolbin1; am101; am101.in; golbin1 */
    { 0x0311161,0x0031DA1, 0x8C,0x80, 0x6,+0 }, /* 102: GM102; HMIGM102; b13M102; b7M102; b8M102; f34GM102; fat2M102; rickM98; sGM102; Echodrp1; FX 7 echoes; am102; am102.in; echodrp1 */
    { 0x173F3A4,0x0238161, 0x4C,0x00, 0x4,+0 }, /* 103: GM103; HMIGM103; b13M103; b7M103; f34GM103; fat2M103; sGM103; FX 8 sci-fi; am103; am103.in */
    { 0x053D202,0x1F6F207, 0x85,0x03, 0x0,+0 }, /* 104: GM104; HMIGM104; b13M104; b7M104; f17GM104; f29GM63; f30GM63; f34GM104; fat2M104; mGM104; sGM104; Sitar; Synth Brass 2; am104; am104.in */
    { 0x111A311,0x0E5A213, 0x0C,0x80, 0x0,+0 }, /* 105: GM105; HMIGM105; b13M105; b7M105; f17GM105; f34GM105; fat2M105; mGM105; sGM105; Banjo; am105; am105.in */
    { 0x141F611,0x2E6F211, 0x06,0x00, 0x4,+0 }, /* 106: GM106; HMIGM106; b13M106; b6M24; b7M106; f17GM106; f34GM106; fat2M106; hamM24; intM24; mGM106; rickM24; sGM106; LDist; LDist.in; Shamisen; am106; am106.in */
    { 0x032D493,0x111EB91, 0x91,0x00, 0x8,+0 }, /* 107: GM107; HMIGM107; b13M107; b7M107; f34GM107; fat2M107; sGM107; Koto; am107; am107.in */
    { 0x056FA04,0x005C201, 0x4F,0x00, 0xC,+0 }, /* 108: BisqM108; GM108; HMIGM108; b13M108; b50M108; b51M108; b7M108; f17GM108; f20GM108; f31GM108; f31GM45; f34GM108; f35GM108; f36GM108; f48GM108; f49GM108; fat2M108; hamM57; mGM108; qGM108; sGM108; Kalimba; Pizzicato String; am108; am108.in; gm108; kalimba */
    { 0x0207C21,0x10C6F22, 0x49,0x00, 0x6,+0 }, /* 109: BisqM109; GM109; HMIGM109; b13M109; b50M109; b51M109; b7M109; f17GM109; f20GM109; f31GM109; f34GM109; f35GM109; f36GM109; f48GM109; f49GM109; fat2M109; mGM109; qGM109; sGM109; Bagpipe; am109; am109.in; gm109 */
    { 0x133DD31,0x0165621, 0x85,0x00, 0xA,+0 }, /* 110: BisqM110; GM110; HMIGM110; b13M110; b50M110; b51M110; b7M110; f17GM110; f20GM110; f31GM110; f34GM110; f35GM110; f36GM110; f48GM110; f49GM110; fat2M110; mGM110; qGM110; sGM110; Fiddle; am110; am110.in; gm110 */
    { 0x205DA20,0x00B8F21, 0x04,0x81, 0x6,+0 }, /* 111: GM111; HMIGM111; b13M111; b7M111; b8M111; f17GM111; f34GM111; f35GM111; fat2M111; mGM111; sGM111; Shanai; am111; am111.in; shanna1 */
    { 0x0E5F105,0x0E5C303, 0x6A,0x80, 0x6,+0 }, /* 112: BisqM112; GM112; HMIGM112; b13M112; b50M112; b51M112; b7M112; f17GM112; f20GM112; f31GM112; f34GM112; f36GM112; f48GM112; fat2M112; mGM112; qGM112; sGM112; Tinkle Bell; am112; am112.in; gm112 */
    { 0x026EC07,0x016F802, 0x15,0x00, 0xA,+0 }, /* 113: GM113; HMIGM113; b13M113; b7M113; f17GM113; f34GM113; f35GM113; fat2M113; hamM50; mGM113; sGM113; Agogo Bells; agogo; am113; am113.in */
    { 0x0356705,0x005DF01, 0x9D,0x00, 0x8,+0 }, /* 114: GM114; HMIGM114; b13M114; b7M114; b8M114; f17GM114; f34GM114; f35GM114; fat2M114; mGM114; sGM114; Steel Drums; am114; am114.in; steeldrm */
    { 0x028FA18,0x0E5F812, 0x96,0x00, 0xA,+0 }, /* 115: GM115; HMIGM115; b13M115; b7M115; b8M115; f17GM115; f34GM115; f35GM115; fat2M115; mGM115; rickM100; sGM115; Woodblk.; Woodblock; am115; am115.in; woodblk */
    { 0x007A810,0x003FA00, 0x86,0x03, 0x6,+0 }, /* 116: BisqM116; GM116; HMIGM116; b13M116; b50M116; b51M116; b7M116; f17GM116; f20GM116; f29GM118; f30GM117; f30GM118; f31GM116; f34GM116; f35GM116; f36GM116; f49GM116; fat2M116; hamM69; hamP90; mGM116; qGM116; Melodic Tom; Synth Drum; Taiko; Taiko Drum; am116; am116.in; gm116; taiko */
    { 0x247F811,0x003F310, 0x41,0x03, 0x4,+0 }, /* 117: GM117; HMIGM117; b13M117; b7M117; b8M117; f17GM117; f29GM113; f30GM113; f34GM117; f35GM117; fat2M117; hamM58; mGM117; sGM117; Agogo Bells; Melodic Tom; am117; am117.in; melotom */
    { 0x206F101,0x002F310, 0x8E,0x00, 0xE,+0 }, /* 118: GM118; HMIGM118; b13M118; b7M118; f17GM118; f34GM118; fat2M118; mGM118; Synth Drum; am118; am118.in */
    { 0x0001F0E,0x3FF1FC0, 0x00,0x00, 0xE,+0 }, /* 119: GM119; HMIGM119; b13M119; b7M119; f34GM119; fat2M119; mGM119; Reverse Cymbal; am119; am119.in */
    { 0x024F806,0x2845603, 0x80,0x88, 0xE,+0 }, /* 120: GM120; HMIGM120; b13M120; b6M36; b7M120; f17GM120; f34GM120; f35GM120; fat2M120; hamM36; intM36; mGM120; rickM101; rickM36; sGM120; DNoise1; DNoise1.; Fretnos.; Guitar FretNoise; am120; am120.in */
    { 0x000F80E,0x30434D0, 0x00,0x05, 0xE,+0 }, /* 121: BisqM121; GM121; HMIGM121; b13M121; b7M121; f17GM121; f34GM121; f35GM121; fat2M121; mGM121; sGM121; Breath Noise; am121; am121.in */
    { 0x000F60E,0x3021FC0, 0x00,0x00, 0xE,+0 }, /* 122: GM122; HMIGM122; b13M122; b7M122; f17GM122; f34GM122; fat2M122; mGM122; sGM122; Seashore; am122; am122.in */
    { 0x0A337D5,0x03756DA, 0x95,0x40, 0x0,+0 }, /* 123: GM123; HMIGM123; b13M123; b7M123; f15GM124; f17GM123; f26GM124; f29GM124; f30GM124; f34GM123; fat2M123; mGM123; sGM123; Bird Tweet; Telephone; am123; am123.in */
    { 0x261B235,0x015F414, 0x5C,0x08, 0xA,+0 }, /* 124: GM124; HMIGM124; b13M124; b7M124; f17GM124; f29GM123; f30GM123; f34GM124; fat2M124; mGM124; sGM124; Bird Tweet; Telephone; am124; am124.in */
    { 0x000F60E,0x3F54FD0, 0x00,0x00, 0xE,+0 }, /* 125: GM125; HMIGM125; b13M125; b7M125; f17GM125; f34GM125; fat2M125; mGM125; sGM125; Helicopter; am125; am125.in */
    { 0x001FF26,0x11612E4, 0x00,0x00, 0xE,+0 }, /* 126: GM126; HMIGM126; b13M126; b7M126; b8M123; f17GM126; f34GM126; f35GM126; fat2M126; mGM126; sGM126; Applause/Noise; am126; am126.in; applaus */
    { 0x0F0F300,0x2C9F600, 0x00,0x00, 0xE,+0 }, /* 127: GM127; HMIGM127; b13M127; b7M127; f17GM127; f34GM127; fat2M127; mGM127; sGM127; Gunshot; am127; am127.in */

    /* 128 percussions follow (known by MIDI as channel #9) */

    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 },
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 }, /* 35: BisqP0; BisqP11; BisqP12; BisqP36; BisqP4; GP35; GP36; b13P0; b13P1; b13P10; b13P11; b13P12; b13P13; b13P14; b13P15; b13P16; b13P17; b13P18; b13P19; b13P2; b13P20; b13P21; b13P22; b13P23; b13P24; b13P25; b13P26; b13P27; b13P28; b13P29; b13P3; b13P30; b13P31; b13P32; b13P33; b13P34; b13P35; b13P36; b13P4; b13P5; b13P6; b13P7; b13P8; b13P9; b50P35; b6P34; b6P35; b6P92; b7P0; b7P1; b7P10; b7P11; b7P12; b7P13; b7P14; b7P15; b7P16; b7P17; b7P18; b7P19; b7P2; b7P20; b7P21; b7P22; b7P23; b7P24; b7P25; b7P26; b7P27; b7P28; b7P29; b7P3; b7P30; b7P31; b7P32; b7P33; b7P34; b7P35; b7P36; b7P4; b7P5; b7P6; b7P7; b7P8; b7P9; b8P34; f17GP35; f17GP36; f20GP35; f20GP36; f29GP35; f29GP36; f30GP35; f30GP36; f31GP31; f31GP35; f31GP36; f34GP35; f34GP36; f35GP35; f42GP36; fat2P35; fat2P36; hamP11; hamP34; hamP35; intP34; intP35; mGP35; mGP36; qGP35; qGP36; rickP14; rickP34; rickP35; Ac Bass Drum; Bass Drum 1; Rkick2; apo035; apo035.i; aps035; aps035.i; gps035; kick2; kick2.in */
    { 0x277F810,0x006F311, 0x44,0x00, 0x8,+0 }, /* 36: BisqP0; BisqP11; BisqP12; BisqP36; BisqP4; GP35; GP36; b13P0; b13P1; b13P10; b13P11; b13P12; b13P13; b13P14; b13P15; b13P16; b13P17; b13P18; b13P19; b13P2; b13P20; b13P21; b13P22; b13P23; b13P24; b13P25; b13P26; b13P27; b13P28; b13P29; b13P3; b13P30; b13P31; b13P32; b13P33; b13P34; b13P35; b13P36; b13P4; b13P5; b13P6; b13P7; b13P8; b13P9; b50P35; b6P34; b6P35; b6P92; b7P0; b7P1; b7P10; b7P11; b7P12; b7P13; b7P14; b7P15; b7P16; b7P17; b7P18; b7P19; b7P2; b7P20; b7P21; b7P22; b7P23; b7P24; b7P25; b7P26; b7P27; b7P28; b7P29; b7P3; b7P30; b7P31; b7P32; b7P33; b7P34; b7P35; b7P36; b7P4; b7P5; b7P6; b7P7; b7P8; b7P9; b8P34; f17GP35; f17GP36; f20GP35; f20GP36; f29GP35; f29GP36; f30GP35; f30GP36; f31GP31; f31GP35; f31GP36; f34GP35; f34GP36; f35GP35; f42GP36; fat2P35; fat2P36; hamP11; hamP34; hamP35; intP34; intP35; mGP35; mGP36; qGP35; qGP36; rickP14; rickP34; rickP35; Ac Bass Drum; Bass Drum 1; Rkick2; apo035; apo035.i; aps035; aps035.i; gps035; kick2; kick2.in */
    { 0x0FFF902,0x0FFF811, 0x07,0x00, 0x8,+0 }, /* 37: GP37; b13P37; b7P37; f17GP37; f23GP54; f29GP37; f30GP37; f34GP37; f49GP37; fat2P37; mGP37; Side Stick; Tambourine; aps037; aps037.i */
    { 0x205FC00,0x017FA00, 0x00,0x00, 0xE,+0 }, /* 38: BisqP40; GP38; GP40; b13P38; b13P40; b50P38; b50P40; b7P38; b7P40; f17GP38; f17GP40; f20GP38; f20GP40; f29GP38; f29GP40; f30GP38; f30GP40; f31GP38; f34GP38; f34GP40; f49GP38; fat2P38; fat2P40; mGP38; mGP40; qGP38; qGP40; Acoustic Snare; Electric Snare; aps038; aps038.i; aps040; aps040.i; gps038; gps040 */
    { 0x007FF00,0x008FF01, 0x02,0x00, 0x0,+0 }, /* 39: GP39; b13P39; b7P39; f17GP39; f29GP39; f30GP39; f34GP39; f49GP39; fat2P39; mGP39; Hand Clap; aps039; aps039.i */
    { 0x205FC00,0x017FA00, 0x00,0x00, 0xE,+0 }, /* 40: BisqP40; GP38; GP40; b13P38; b13P40; b50P38; b50P40; b7P38; b7P40; f17GP38; f17GP40; f20GP38; f20GP40; f29GP38; f29GP40; f30GP38; f30GP40; f31GP38; f34GP38; f34GP40; f49GP38; fat2P38; fat2P40; mGP38; mGP40; qGP38; qGP40; Acoustic Snare; Electric Snare; aps038; aps038.i; aps040; aps040.i; gps038; gps040 */
    { 0x00CF600,0x006F600, 0x00,0x00, 0x4,+0 }, /* 41: BisqP1; BisqP105; BisqP87; GP41; GP43; GP45; GP47; GP48; GP50; GP87; b13P100; b13P101; b13P102; b13P103; b13P104; b13P105; b13P106; b13P107; b13P108; b13P109; b13P110; b13P111; b13P112; b13P113; b13P114; b13P115; b13P116; b13P117; b13P118; b13P119; b13P120; b13P121; b13P122; b13P123; b13P124; b13P125; b13P126; b13P127; b13P41; b13P43; b13P45; b13P47; b13P48; b13P50; b13P87; b13P88; b13P89; b13P90; b13P91; b13P92; b13P93; b13P94; b13P95; b13P96; b13P97; b13P98; b13P99; b7P100; b7P101; b7P102; b7P103; b7P104; b7P105; b7P106; b7P107; b7P108; b7P109; b7P110; b7P111; b7P112; b7P113; b7P114; b7P115; b7P116; b7P117; b7P118; b7P119; b7P120; b7P121; b7P122; b7P123; b7P124; b7P125; b7P126; b7P127; b7P41; b7P43; b7P45; b7P47; b7P48; b7P50; b7P87; b7P88; b7P89; b7P90; b7P91; b7P92; b7P93; b7P94; b7P95; b7P96; b7P97; b7P98; b7P99; b8P87; f17GP41; f17GP43; f17GP45; f17GP47; f17GP48; f17GP50; f17GP87; f29GP41; f29GP43; f29GP45; f29GP47; f29GP48; f29GP50; f29GP87; f30GP41; f30GP43; f30GP45; f30GP47; f30GP48; f30GP50; f30GP87; f34GP41; f34GP43; f34GP45; f34GP47; f34GP48; f34GP50; f34GP87; f35GP41; f35GP43; f35GP45; f35GP47; f35GP48; f35GP50; f35GP87; f42GP41; f42GP43; f42GP45; f42GP47; f42GP48; f42GP50; f49GP41; f49GP43; f49GP45; f49GP47; f49GP48; f49GP50; f49GP87; fat2P41; fat2P43; fat2P45; fat2P47; fat2P48; fat2P50; fat2P87; hamP1; hamP2; hamP3; hamP4; hamP5; hamP6; mGP41; mGP43; mGP45; mGP47; mGP48; mGP50; mGP87; rickP105; sGP87; High Floor Tom; High Tom; High-Mid Tom; Low Floor Tom; Low Tom; Low-Mid Tom; Open Surdu; aps041; aps041.i; aps087; aps087.i; surdu.in; surduo */
    { 0x008F60C,0x247FB12, 0x00,0x00, 0xA,+0 }, /* 42: GP42; b13P42; b50P42; b6P55; b7P42; dukeP42; f17GP42; f20GP42; f23GP68; f23GP70; f29GP42; f30GP42; f31GP42; f34GP1; f34GP42; fat2P42; hamP55; intP55; mGP42; qGP42; rickP55; swP42; Closed High Hat; Low Agogo; Maracas; aps042; aps042.i; gps042 */
    { 0x00CF600,0x006F600, 0x00,0x00, 0x4,+0 }, /* 43: BisqP1; BisqP105; BisqP87; GP41; GP43; GP45; GP47; GP48; GP50; GP87; b13P100; b13P101; b13P102; b13P103; b13P104; b13P105; b13P106; b13P107; b13P108; b13P109; b13P110; b13P111; b13P112; b13P113; b13P114; b13P115; b13P116; b13P117; b13P118; b13P119; b13P120; b13P121; b13P122; b13P123; b13P124; b13P125; b13P126; b13P127; b13P41; b13P43; b13P45; b13P47; b13P48; b13P50; b13P87; b13P88; b13P89; b13P90; b13P91; b13P92; b13P93; b13P94; b13P95; b13P96; b13P97; b13P98; b13P99; b7P100; b7P101; b7P102; b7P103; b7P104; b7P105; b7P106; b7P107; b7P108; b7P109; b7P110; b7P111; b7P112; b7P113; b7P114; b7P115; b7P116; b7P117; b7P118; b7P119; b7P120; b7P121; b7P122; b7P123; b7P124; b7P125; b7P126; b7P127; b7P41; b7P43; b7P45; b7P47; b7P48; b7P50; b7P87; b7P88; b7P89; b7P90; b7P91; b7P92; b7P93; b7P94; b7P95; b7P96; b7P97; b7P98; b7P99; b8P87; f17GP41; f17GP43; f17GP45; f17GP47; f17GP48; f17GP50; f17GP87; f29GP41; f29GP43; f29GP45; f29GP47; f29GP48; f29GP50; f29GP87; f30GP41; f30GP43; f30GP45; f30GP47; f30GP48; f30GP50; f30GP87; f34GP41; f34GP43; f34GP45; f34GP47; f34GP48; f34GP50; f34GP87; f35GP41; f35GP43; f35GP45; f35GP47; f35GP48; f35GP50; f35GP87; f42GP41; f42GP43; f42GP45; f42GP47; f42GP48; f42GP50; f49GP41; f49GP43; f49GP45; f49GP47; f49GP48; f49GP50; f49GP87; fat2P41; fat2P43; fat2P45; fat2P47; fat2P48; fat2P50; fat2P87; hamP1; hamP2; hamP3; hamP4; hamP5; hamP6; mGP41; mGP43; mGP45; mGP47; mGP48; mGP50; mGP87; rickP105; sGP87; High Floor Tom; High Tom; High-Mid Tom; Low Floor Tom; Low Tom; Low-Mid Tom; Open Surdu; aps041; aps041.i; aps087; aps087.i; surdu.in; surduo */
    { 0x008F60C,0x2477B12, 0x00,0x05, 0xA,+0 }, /* 44: GP44; b13P44; b6P102; b7P44; b8P44; f17GP44; f29GP44; f30GP44; f34GP44; f35GP44; f49GP44; fat2P44; mGP44; Pedal High Hat; Rpedhhat; aps044; aps044.i; pedalhht */
    { 0x00CF600,0x006F600, 0x00,0x00, 0x4,+0 }, /* 45: BisqP1; BisqP105; BisqP87; GP41; GP43; GP45; GP47; GP48; GP50; GP87; b13P100; b13P101; b13P102; b13P103; b13P104; b13P105; b13P106; b13P107; b13P108; b13P109; b13P110; b13P111; b13P112; b13P113; b13P114; b13P115; b13P116; b13P117; b13P118; b13P119; b13P120; b13P121; b13P122; b13P123; b13P124; b13P125; b13P126; b13P127; b13P41; b13P43; b13P45; b13P47; b13P48; b13P50; b13P87; b13P88; b13P89; b13P90; b13P91; b13P92; b13P93; b13P94; b13P95; b13P96; b13P97; b13P98; b13P99; b7P100; b7P101; b7P102; b7P103; b7P104; b7P105; b7P106; b7P107; b7P108; b7P109; b7P110; b7P111; b7P112; b7P113; b7P114; b7P115; b7P116; b7P117; b7P118; b7P119; b7P120; b7P121; b7P122; b7P123; b7P124; b7P125; b7P126; b7P127; b7P41; b7P43; b7P45; b7P47; b7P48; b7P50; b7P87; b7P88; b7P89; b7P90; b7P91; b7P92; b7P93; b7P94; b7P95; b7P96; b7P97; b7P98; b7P99; b8P87; f17GP41; f17GP43; f17GP45; f17GP47; f17GP48; f17GP50; f17GP87; f29GP41; f29GP43; f29GP45; f29GP47; f29GP48; f29GP50; f29GP87; f30GP41; f30GP43; f30GP45; f30GP47; f30GP48; f30GP50; f30GP87; f34GP41; f34GP43; f34GP45; f34GP47; f34GP48; f34GP50; f34GP87; f35GP41; f35GP43; f35GP45; f35GP47; f35GP48; f35GP50; f35GP87; f42GP41; f42GP43; f42GP45; f42GP47; f42GP48; f42GP50; f49GP41; f49GP43; f49GP45; f49GP47; f49GP48; f49GP50; f49GP87; fat2P41; fat2P43; fat2P45; fat2P47; fat2P48; fat2P50; fat2P87; hamP1; hamP2; hamP3; hamP4; hamP5; hamP6; mGP41; mGP43; mGP45; mGP47; mGP48; mGP50; mGP87; rickP105; sGP87; High Floor Tom; High Tom; High-Mid Tom; Low Floor Tom; Low Tom; Low-Mid Tom; Open Surdu; aps041; aps041.i; aps087; aps087.i; surdu.in; surduo */
    { 0x002F60C,0x243CB12, 0x00,0x00, 0xA,+0 }, /* 46: GP46; b13P46; b50P46; b7P46; f17GP46; f20GP46; f29GP46; f30GP46; f31GP46; f34GP46; f49GP46; fat2P46; mGP46; qGP46; Open High Hat; aps046; aps046.i; gps046 */
    { 0x00CF600,0x006F600, 0x00,0x00, 0x4,+0 }, /* 47: BisqP1; BisqP105; BisqP87; GP41; GP43; GP45; GP47; GP48; GP50; GP87; b13P100; b13P101; b13P102; b13P103; b13P104; b13P105; b13P106; b13P107; b13P108; b13P109; b13P110; b13P111; b13P112; b13P113; b13P114; b13P115; b13P116; b13P117; b13P118; b13P119; b13P120; b13P121; b13P122; b13P123; b13P124; b13P125; b13P126; b13P127; b13P41; b13P43; b13P45; b13P47; b13P48; b13P50; b13P87; b13P88; b13P89; b13P90; b13P91; b13P92; b13P93; b13P94; b13P95; b13P96; b13P97; b13P98; b13P99; b7P100; b7P101; b7P102; b7P103; b7P104; b7P105; b7P106; b7P107; b7P108; b7P109; b7P110; b7P111; b7P112; b7P113; b7P114; b7P115; b7P116; b7P117; b7P118; b7P119; b7P120; b7P121; b7P122; b7P123; b7P124; b7P125; b7P126; b7P127; b7P41; b7P43; b7P45; b7P47; b7P48; b7P50; b7P87; b7P88; b7P89; b7P90; b7P91; b7P92; b7P93; b7P94; b7P95; b7P96; b7P97; b7P98; b7P99; b8P87; f17GP41; f17GP43; f17GP45; f17GP47; f17GP48; f17GP50; f17GP87; f29GP41; f29GP43; f29GP45; f29GP47; f29GP48; f29GP50; f29GP87; f30GP41; f30GP43; f30GP45; f30GP47; f30GP48; f30GP50; f30GP87; f34GP41; f34GP43; f34GP45; f34GP47; f34GP48; f34GP50; f34GP87; f35GP41; f35GP43; f35GP45; f35GP47; f35GP48; f35GP50; f35GP87; f42GP41; f42GP43; f42GP45; f42GP47; f42GP48; f42GP50; f49GP41; f49GP43; f49GP45; f49GP47; f49GP48; f49GP50; f49GP87; fat2P41; fat2P43; fat2P45; fat2P47; fat2P48; fat2P50; fat2P87; hamP1; hamP2; hamP3; hamP4; hamP5; hamP6; mGP41; mGP43; mGP45; mGP47; mGP48; mGP50; mGP87; rickP105; sGP87; High Floor Tom; High Tom; High-Mid Tom; Low Floor Tom; Low Tom; Low-Mid Tom; Open Surdu; aps041; aps041.i; aps087; aps087.i; surdu.in; surduo */
    { 0x00CF600,0x006F600, 0x00,0x00, 0x4,+0 }, /* 48: BisqP1; BisqP105; BisqP87; GP41; GP43; GP45; GP47; GP48; GP50; GP87; b13P100; b13P101; b13P102; b13P103; b13P104; b13P105; b13P106; b13P107; b13P108; b13P109; b13P110; b13P111; b13P112; b13P113; b13P114; b13P115; b13P116; b13P117; b13P118; b13P119; b13P120; b13P121; b13P122; b13P123; b13P124; b13P125; b13P126; b13P127; b13P41; b13P43; b13P45; b13P47; b13P48; b13P50; b13P87; b13P88; b13P89; b13P90; b13P91; b13P92; b13P93; b13P94; b13P95; b13P96; b13P97; b13P98; b13P99; b7P100; b7P101; b7P102; b7P103; b7P104; b7P105; b7P106; b7P107; b7P108; b7P109; b7P110; b7P111; b7P112; b7P113; b7P114; b7P115; b7P116; b7P117; b7P118; b7P119; b7P120; b7P121; b7P122; b7P123; b7P124; b7P125; b7P126; b7P127; b7P41; b7P43; b7P45; b7P47; b7P48; b7P50; b7P87; b7P88; b7P89; b7P90; b7P91; b7P92; b7P93; b7P94; b7P95; b7P96; b7P97; b7P98; b7P99; b8P87; f17GP41; f17GP43; f17GP45; f17GP47; f17GP48; f17GP50; f17GP87; f29GP41; f29GP43; f29GP45; f29GP47; f29GP48; f29GP50; f29GP87; f30GP41; f30GP43; f30GP45; f30GP47; f30GP48; f30GP50; f30GP87; f34GP41; f34GP43; f34GP45; f34GP47; f34GP48; f34GP50; f34GP87; f35GP41; f35GP43; f35GP45; f35GP47; f35GP48; f35GP50; f35GP87; f42GP41; f42GP43; f42GP45; f42GP47; f42GP48; f42GP50; f49GP41; f49GP43; f49GP45; f49GP47; f49GP48; f49GP50; f49GP87; fat2P41; fat2P43; fat2P45; fat2P47; fat2P48; fat2P50; fat2P87; hamP1; hamP2; hamP3; hamP4; hamP5; hamP6; mGP41; mGP43; mGP45; mGP47; mGP48; mGP50; mGP87; rickP105; sGP87; High Floor Tom; High Tom; High-Mid Tom; Low Floor Tom; Low Tom; Low-Mid Tom; Open Surdu; aps041; aps041.i; aps087; aps087.i; surdu.in; surduo */
    { 0x000F60E,0x3029FD0, 0x00,0x00, 0xE,+0 }, /* 49: BisqP49; GP49; GP57; b13P49; b13P57; b6P112; b7P49; b7P57; b8P49; f15GP49; f17GP49; f17GP57; f26GP49; f29GP49; f29GP57; f30GP49; f30GP57; f34GP49; f34GP57; f35GP49; f49GP49; f49GP57; fat2P49; fat2P57; hamP0; mGP49; mGP57; oGP49; Crash Cymbal 1; Crash Cymbal 2; Rcrash1; aps049; aps049.i; aps057; aps057.i; crash1 */
    { 0x00CF600,0x006F600, 0x00,0x00, 0x4,+0 }, /* 50: BisqP1; BisqP105; BisqP87; GP41; GP43; GP45; GP47; GP48; GP50; GP87; b13P100; b13P101; b13P102; b13P103; b13P104; b13P105; b13P106; b13P107; b13P108; b13P109; b13P110; b13P111; b13P112; b13P113; b13P114; b13P115; b13P116; b13P117; b13P118; b13P119; b13P120; b13P121; b13P122; b13P123; b13P124; b13P125; b13P126; b13P127; b13P41; b13P43; b13P45; b13P47; b13P48; b13P50; b13P87; b13P88; b13P89; b13P90; b13P91; b13P92; b13P93; b13P94; b13P95; b13P96; b13P97; b13P98; b13P99; b7P100; b7P101; b7P102; b7P103; b7P104; b7P105; b7P106; b7P107; b7P108; b7P109; b7P110; b7P111; b7P112; b7P113; b7P114; b7P115; b7P116; b7P117; b7P118; b7P119; b7P120; b7P121; b7P122; b7P123; b7P124; b7P125; b7P126; b7P127; b7P41; b7P43; b7P45; b7P47; b7P48; b7P50; b7P87; b7P88; b7P89; b7P90; b7P91; b7P92; b7P93; b7P94; b7P95; b7P96; b7P97; b7P98; b7P99; b8P87; f17GP41; f17GP43; f17GP45; f17GP47; f17GP48; f17GP50; f17GP87; f29GP41; f29GP43; f29GP45; f29GP47; f29GP48; f29GP50; f29GP87; f30GP41; f30GP43; f30GP45; f30GP47; f30GP48; f30GP50; f30GP87; f34GP41; f34GP43; f34GP45; f34GP47; f34GP48; f34GP50; f34GP87; f35GP41; f35GP43; f35GP45; f35GP47; f35GP48; f35GP50; f35GP87; f42GP41; f42GP43; f42GP45; f42GP47; f42GP48; f42GP50; f49GP41; f49GP43; f49GP45; f49GP47; f49GP48; f49GP50; f49GP87; fat2P41; fat2P43; fat2P45; fat2P47; fat2P48; fat2P50; fat2P87; hamP1; hamP2; hamP3; hamP4; hamP5; hamP6; mGP41; mGP43; mGP45; mGP47; mGP48; mGP50; mGP87; rickP105; sGP87; High Floor Tom; High Tom; High-Mid Tom; Low Floor Tom; Low Tom; Low-Mid Tom; Open Surdu; aps041; aps041.i; aps087; aps087.i; surdu.in; surduo */
    { 0x042F80E,0x3E4F407, 0x08,0x4A, 0xE,+0 }, /* 51: GP51; GP59; b13P51; b13P59; b6P111; b7P51; b7P59; b8P51; b8P59; f17GP51; f17GP59; f29GM119; f29GM125; f29GM127; f29GP51; f29GP59; f30GM119; f30GM125; f30GM127; f30GP51; f30GP59; f34GP51; f34GP59; f35GP51; f35GP59; f49GP51; f49GP59; fat2P51; fat2P59; mGP51; mGP59; sGP51; sGP59; Gunshot; Helicopter; Reverse Cymbal; Ride Cymbal 1; Ride Cymbal 2; Rridecym; aps051; aps051.i; ridecym */
    { 0x030F50E,0x0029FD0, 0x00,0x0A, 0xE,+0 }, /* 52: GP52; b13P52; b7P52; b8P52; f17GP52; f29GP52; f30GP52; f34GP52; f35GP52; f49GP52; fat2P52; hamP19; mGP52; Chinese Cymbal; aps052; aps052.i; cymchin */
    { 0x3E4E40E,0x1E5F507, 0x0A,0x5D, 0x6,+0 }, /* 53: GP53; b13P53; b7P53; b8P53; f17GP53; f29GP53; f30GP53; f34GP53; f35GP53; f49GP53; fat2P53; mGP53; sGP53; Ride Bell; aps053; aps053.i; ridebell */
    { 0x004B402,0x0F79705, 0x03,0x0A, 0xE,+0 }, /* 54: GP54; b13P54; b7P54; dukeP46; f17GP54; f30GP54; f34GP54; f49GP54; fat2P54; mGP54; swP46; Open High Hat; Tambourine; aps054; aps054.i */
    { 0x000F64E,0x3029F9E, 0x00,0x00, 0xE,+0 }, /* 55: GP55; b13P55; b6P110; b7P55; b8P55; f34GP55; fat2P55; Rsplash; Splash Cymbal; aps055; aps055.i; cysplash */
    { 0x237F811,0x005F310, 0x45,0x08, 0x8,+0 }, /* 56: GP56; b13P56; b7P56; f17GP56; f29GP56; f30GP56; f34GP56; f48GP56; f49GP56; fat2P56; mGP56; sGP56; Cow Bell; aps056; aps056.i */
    { 0x000F60E,0x3029FD0, 0x00,0x00, 0xE,+0 }, /* 57: BisqP49; GP49; GP57; b13P49; b13P57; b6P112; b7P49; b7P57; b8P49; f15GP49; f17GP49; f17GP57; f26GP49; f29GP49; f29GP57; f30GP49; f30GP57; f34GP49; f34GP57; f35GP49; f49GP49; f49GP57; fat2P49; fat2P57; hamP0; mGP49; mGP57; oGP49; Crash Cymbal 1; Crash Cymbal 2; Rcrash1; aps049; aps049.i; aps057; aps057.i; crash1 */
    { 0x303FF80,0x014FF10, 0x00,0x0D, 0xC,+0 }, /* 58: GP58; b13P58; b7P58; b8P58; f34GP58; fat2P58; Vibraslap; aps058; aps058.i; vibra */
    { 0x042F80E,0x3E4F407, 0x08,0x4A, 0xE,+0 }, /* 59: GP51; GP59; b13P51; b13P59; b6P111; b7P51; b7P59; b8P51; b8P59; f17GP51; f17GP59; f29GM119; f29GM125; f29GM127; f29GP51; f29GP59; f30GM119; f30GM125; f30GM127; f30GP51; f30GP59; f34GP51; f34GP59; f35GP51; f35GP59; f49GP51; f49GP59; fat2P51; fat2P59; mGP51; mGP59; sGP51; sGP59; Gunshot; Helicopter; Reverse Cymbal; Ride Cymbal 1; Ride Cymbal 2; Rridecym; aps051; aps051.i; ridecym */
    { 0x00CF506,0x008F502, 0x0B,0x00, 0x6,+0 }, /* 60: GP60; b13P60; b7P60; f17GP60; f29GP60; f30GP60; f34GP60; f48GP60; f49GP60; fat2P60; mGP60; sGP60; High Bongo; aps060; aps060.i */
    { 0x0BFFA01,0x097C802, 0x00,0x00, 0x7,+0 }, /* 61: GP61; b13P61; b7P61; dukeP61; f15GP61; f17GP61; f26GP61; f29GP61; f30GP61; f34GP61; f48GP61; f49GP61; fat2P61; mGP61; oGP61; sGP61; swP61; Low Bongo; aps061; aps061.i */
    { 0x087FA01,0x0B7FA01, 0x51,0x00, 0x6,+0 }, /* 62: GP62; b13P62; b50P62; b7P62; f17GP62; f20GP62; f29GP62; f30GP62; f31GP62; f34GP62; f48GP62; f49GP62; fat2P62; mGP62; qGP62; sGP62; Mute High Conga; aps062; aps062.i; gps062 */
    { 0x08DFA01,0x0B8F802, 0x54,0x00, 0x6,+0 }, /* 63: GP63; b13P63; b7P63; f17GP63; f29GP63; f30GP63; f34GP63; f48GP63; f49GP63; fat2P63; mGP63; sGP63; Open High Conga; aps063; aps063.i */
    { 0x088FA01,0x0B6F802, 0x59,0x00, 0x6,+0 }, /* 64: GP64; b13P64; b7P64; f17GP64; f29GP64; f30GP64; f34GP64; f48GP64; f49GP64; fat2P64; mGP64; sGP64; Low Conga; aps064; aps064.i */
    { 0x30AF901,0x006FA00, 0x00,0x00, 0xE,+0 }, /* 65: BisqP98; BisqP99; GP65; b13P65; b6P115; b7P65; b8P65; b8P66; f17GP65; f29GP65; f30GP65; f34GP65; f35GP65; f35GP66; f48GP65; f49GP65; fat2P65; hamP8; mGP65; rickP98; rickP99; sGP65; High Timbale; Low Timbale; Rtimbale; aps065; aps065.i; timbale; timbale. */
    { 0x389F900,0x06CF600, 0x80,0x00, 0xE,+0 }, /* 66: GP66; b13P66; b50P66; b7P66; f17GP66; f20GP66; f30GP66; f31GP66; f34GP66; f48GP66; f49GP66; fat2P66; mGP66; qGP66; sGP66; Low Timbale; aps066; aps066.i; gps066 */
    { 0x388F803,0x0B6F60C, 0x80,0x08, 0xF,+0 }, /* 67: GP67; b13P67; b7P67; b8P67; b8P68; f17GP67; f29GP67; f30GP67; f34GP67; f35GP67; f35GP68; f49GP67; fat2P67; mGP67; sGP67; High Agogo; Low Agogo; agogo1; aps067; aps067.i */
    { 0x388F803,0x0B6F60C, 0x85,0x00, 0xF,+0 }, /* 68: GP68; b13P68; b7P68; f17GP68; f29GP68; f30GP68; f34GP68; f49GP68; fat2P68; mGP68; sGP68; Low Agogo; aps068; aps068.i */
    { 0x04F760E,0x2187700, 0x40,0x08, 0xE,+0 }, /* 69: GP69; b13P69; b7P69; dukeP44; dukeP69; dukeP82; f15GP69; f17GP69; f26GP69; f29GP69; f30GP69; f34GP69; f42GP69; f49GP69; fat2P69; mGP69; swP44; swP69; swP82; Cabasa; Pedal High Hat; Shaker; aps069; aps069.i */
    { 0x049C80E,0x2699B03, 0x40,0x00, 0xE,+0 }, /* 70: GP70; b13P70; b6P117; b7P70; b8P70; f15GP70; f17GP70; f26GP70; f29GP70; f30GP70; f34GP70; f35GP70; f49GP70; fat2P70; mGP70; sGP70; Maracas; Rmaracas; aps070; aps070.i; maracas */
    { 0x305ADD7,0x0058DC7, 0xDC,0x00, 0xE,+0 }, /* 71: GP71; b13P71; b7P71; b8P71; f15GP71; f17GP71; f26GP71; f29GP71; f30GP71; f34GP71; f35GP71; f48GP71; f49GP71; fat2P71; mGP71; sGP71; Short Whistle; aps071; aps071.i; whistsh */
    { 0x304A8D7,0x00488C7, 0xDC,0x00, 0xE,+0 }, /* 72: GP72; b13P72; b7P72; b8P72; f15GP72; f17GP72; f26GP72; f29GP72; f30GP72; f34GP72; f35GP72; f48GP72; f49GP72; fat2P72; mGP72; sGP72; Long Whistle; aps072; aps072.i; whistll */
    { 0x306F680,0x3176711, 0x00,0x00, 0xE,+0 }, /* 73: BisqP96; GP73; b13P73; b7P73; b8P73; f34GP73; fat2P73; rickP96; sGP73; Short Guiro; aps073; aps073.i; guiros.i; sguiro */
    { 0x205F580,0x3164611, 0x00,0x09, 0xE,+0 }, /* 74: GP74; b13P74; b7P74; f34GP74; fat2P74; sGP74; Long Guiro; aps074; aps074.i */
    { 0x0F40006,0x0F5F715, 0x3F,0x00, 0x1,+0 }, /* 75: GP75; b13P75; b7P75; dukeP75; f15GP75; f17GP75; f26GP75; f29GP75; f30GP75; f34GP75; f49GP75; fat2P75; mGP75; oGP75; sGP75; swP75; Claves; aps075; aps075.i */
    { 0x3F40006,0x0F5F712, 0x3F,0x00, 0x0,+0 }, /* 76: GP76; b13P76; b50P76; b50P77; b7P76; b8P76; f17GP76; f20GP76; f20GP77; f29GP76; f30GP76; f31GP76; f31GP77; f34GP76; f35GP76; f48GP76; f49GP76; fat2P76; mGP76; qGP76; qGP77; High Wood Block; Low Wood Block; aps076; aps076.i; blockhi; gps076; gps077 */
    { 0x0F40006,0x0F5F712, 0x3F,0x00, 0x1,+0 }, /* 77: GP77; b13P77; b7P77; b8P77; f17GP77; f29GP77; f30GP77; f34GP77; f35GP77; f48GP77; f49GP77; fat2P77; mGP77; sGP77; Low Wood Block; aps077; aps077.i; blocklow */
    { 0x0E76701,0x0077502, 0x58,0x00, 0x0,+0 }, /* 78: BisqP78; GP78; b13P78; b7P78; b8P78; f17GP78; f29GP78; f30GP78; f34GP78; f35GP78; f49GP78; fat2P78; mGP78; sGP78; Mute Cuica; aps078; aps078.i; cuicam */
    { 0x048F841,0x0057542, 0x45,0x08, 0x0,+0 }, /* 79: BisqP79; GP79; b13P79; b7P79; b8P79; f34GP79; fat2P79; sGP79; Open Cuica; aps079; aps079.i; cuicao */
    { 0x3F0E00A,0x005FF1E, 0x40,0x4E, 0x8,+0 }, /* 80: GP80; b13P80; b7P80; b8P80; f17GP80; f29GP80; f30GP80; f34GP80; f35GP80; f49GP80; fat2P80; mGP80; sGP80; Mute Triangle; aps080; aps080.i; trianglm */
    { 0x3F0E00A,0x002FF1E, 0x7C,0x52, 0x8,+0 }, /* 81: GP81; b13P81; b7P81; b8P81; f17GP81; f29GM121; f29GP81; f30GM121; f30GP81; f34GP81; f35GP81; f49GP81; fat2P81; mGP81; sGP81; Breath Noise; Open Triangle; aps081; aps081.i; trianglo */
    { 0x04A7A0E,0x21B7B00, 0x40,0x08, 0xE,+0 }, /* 82: GP82; b13P82; b6P116; b7P82; b8P82; f17GP82; f29GP82; f30GP82; f34GP82; f35GP82; f42GP82; f49GP82; fat2P82; hamP7; mGP82; sGP82; Rshaker; Shaker; aps082; aps082.i; shaker */
    { 0x3E4E40E,0x1395507, 0x0A,0x40, 0x6,+0 }, /* 83: GP83; b13P83; b7P83; b8P83; f17GP83; f29GP83; f30GP83; f34GP83; f35GP83; f48GP83; f49GP83; fat2P83; mGP83; sGP83; Jingle Bell; aps083; aps083.i; jingbell */
    { 0x332F905,0x0A5D604, 0x05,0x40, 0xE,+0 }, /* 84: GP84; b13P84; b7P84; b8P84; f15GP51; f17GP84; f26GP51; f29GM126; f29GP84; f30GM126; f30GP84; f34GP84; f35GP84; f49GP84; fat2P84; mGP84; sGP84; Applause/Noise; Bell Tree; Ride Cymbal 1; aps084; aps084.i; belltree */
    { 0x3F30002,0x0F5F715, 0x3F,0x00, 0x8,+0 }, /* 85: GP85; b13P85; b7P85; b8P85; f17GP85; f29GM120; f29GP85; f30GM120; f30GP85; f34GP85; f35GP85; f48GP85; f49GP85; fat2P85; mGP85; sGP85; Castanets; Guitar FretNoise; aps085; aps085.i; castanet */
    { 0x08DFA01,0x0B5F802, 0x4F,0x00, 0x7,+0 }  /* 86: BisqP86; GP86; b13P86; b7P86; b8P86; f15GP63; f15GP64; f17GP86; f26GP63; f26GP64; f29GP86; f30GP86; f34GP86; f35GP86; f49GP86; fat2P86; mGP86; sGP86; Low Conga; Mute Surdu; Open High Conga; aps086; aps086.i; surdum */

};
