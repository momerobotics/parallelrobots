# Parallel Robots - felügyelt tesztelés

Az angol elsődleges dokumentum: [`HANDOFF.md`](HANDOFF.md).

Ez egy kutatási prototípus, nem késztermék. Csak felügyelet mellett használható.
A két motor vezeték nélkül tükrözi egymás elfordulását, rugós ellenállással.

## Indítás előtt

- Az A és B egység külön 3 cellás LiPo akkumulátort használ.
- Csak feltöltött, sérülésmentes, védett akkumulátort használj.
- A mechanika mozgásútja legyen szabad; ujj, haj, ruha és kábel ne kerüljön bele.
- A motorokat ne fogd le, és az első 8 másodpercben ne mozgasd.
- Az alaplapi RGB LED gyártási hiba miatt nem működik; a sötét LED nem jelent hibát.

## Bekapcsolás

1. Tedd mindkét mechanikát kényelmes, szabad kezdőhelyzetbe.
2. Kapcsold be vagy csatlakoztasd az A egység akkumulátorát.
3. Kapcsold be vagy csatlakoztasd a B egység akkumulátorát.
4. Ne érj a mechanikához 8 másodpercig.
5. Ezután az egyik kart csak néhány fokkal mozgasd meg. A másiknak finoman
   követnie kell, rugós ellenállással.

Ha 15 másodperc után sincs ellenállás, kapcsold ki mindkét egységet. Ne próbáld
mozgatással vagy lefogással "felébreszteni".

## Használat

- Egy tesztciklus legfeljebb 10 perc; ezután a firmware automatikusan leállítja
  mindkét motort.
- A javasolt elfordítás legfeljebb kb. 180-200 fok a kezdőhelyzethez képest.
- Ne pörgesd több teljes fordulaton át, és ne tartsd tartósan végállásban.
- Ne fogd le a motort 2 másodpercnél tovább.
- Normális: csendes, enyhén rugós vagy finoman darabos érzet.
- Nem normális: erős rezgés, csattogás, égett szag, forró motor vagy vezérlő,
  hirtelen erős rántás.

## Leállítás és hiba

1. Engedd el a mechanikát.
2. Kapcsold ki vagy válaszd le mindkét akkumulátort.
3. Újabb teszt előtt várj legalább 5 percet.

Kommunikációs hiba után a rendszer reteszelten leáll, és nem indul újra magától.
Az újraindításhoz mindkét egységet ki kell kapcsolni, majd újra be kell kapcsolni.
Rendellenes zaj, rezgés vagy melegedés után ne indítsd újra; jelezd a fejlesztőnek.

## Fontos

- Töltés előtt az akkumulátort mindig válaszd le a vezérlőről.
- A két akkumulátort ne kösd össze egymással.
- Szervizeléshez USB-A - USB-C kábel használható. USB-C - USB-C kábellel ez a
  boardverzió nem kapcsol be megbízhatóan.
- A prototípust ne hagyd bekapcsolva felügyelet nélkül.

Firmware: `MKS_Parallel_Mirror`, handoff mode, protocol 7.
