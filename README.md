# termgrid

termcap の制御と Gridインタフェース。
utf-8用。

## samples

### cursor_move

### unicode_view

keymap

* `h, j, k, l`
* `,` 
* `.`

```
    │00│01│02│03│04│05│06│07│08│09│0a│0b│0c│0d│0e│0f│Unicode PLANE: 0
0000│  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │Basic Latin
0010│  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │Basic Latin
0020│  │! │" │# │$ │% │& │' │( │) │* │+ │, │- │. │/ │Basic Latin
0030│0 │1 │2 │3 │4 │5 │6 │7 │8 │9 │: │; │< │= │> │? │Basic Latin
0040│@ │A │B │C │D │E │F │G │H │I │J │K │L │M │N │O │Basic Latin
0050│P │Q │R │S │T │U │V │W │X │Y │Z │[ │\ │] │^ │_ │Basic Latin
0060│` │a │b │c │d │e │f │g │h │i │j │k │l │m │n │o │Basic Latin
0070│p │q │r │s │t │u │v │w │x │y │z │{ │| │} │~ │  │Basic Latin
0080│  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │Latin-1 Supplement
0090│  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │Latin-1 Supplement
00a0│  │¡ │¢ │£ │¤ │¥ │¦ │§ │¨ │© │ª │« │¬ │­ │® │¯ │Latin-1 Supplement
00b0│° │± │² │³ │´ │µ │¶ │· │¸ │¹ │º │» │¼ │½ │¾ │¿ │Latin-1 Supplement
00c0│À │Á │Â │Ã │Ä │Å │Æ │Ç │È │É │Ê │Ë │Ì │Í │Î │Ï │Latin-1 Supplement
00d0│Ð │Ñ │Ò │Ó │Ô │Õ │Ö │× │Ø │Ù │Ú │Û │Ü │Ý │Þ │ß │Latin-1 Supplement
00e0│à │á │â │ã │ä │å │æ │ç │è │é │ê │ë │ì │í │î │ï │Latin-1 Supplement
00f0│ð │ñ │ò │ó │ô │õ │ö │÷ │ø │ù │ú │û │ü │ý │þ │ÿ │Latin-1 Supplement
0100│Ā │ā │Ă │ă │Ą │ą │Ć │ć │Ĉ │ĉ │Ċ │ċ │Č │č │Ď │ď │Latin Extended-A
0110│Đ │đ │Ē │ē │Ĕ │ĕ │Ė │ė │Ę │ę │Ě │ě │Ĝ │ĝ │Ğ │ğ │Latin Extended-A
0120│Ġ │ġ │Ģ │ģ │Ĥ │ĥ │Ħ │ħ │Ĩ │ĩ │Ī │ī │Ĭ │ĭ │Į │į │Latin Extended-A
0130│İ │ı │Ĳ │ĳ │Ĵ │ĵ │Ķ │ķ │ĸ │Ĺ │ĺ │Ļ │ļ │Ľ │ľ │Ŀ │Latin Extended-A
0140│ŀ │Ł │ł │Ń │ń │Ņ │ņ │Ň │ň │ŉ │Ŋ │ŋ │Ō │ō │Ŏ │ŏ │Latin Extended-A
```

## TODO

* [ ] viewport
* [ ] cursor interface
* [ ] scroll interface
* [ ] buffer / render to tputs
