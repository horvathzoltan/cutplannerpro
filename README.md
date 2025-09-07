# CutPlanner — Anyagoptimalizáló vágó rendszer  
🪚 Qt-alapú alkalmazás rudanyagok hatékony feldolgozására, MVP architektúrával és modern C++20 támogatással.

---

## 🚀 Fő tulajdonságok

- ✂️ Darabigények megadása: hossz és darabszám szerint
- ⚙️ Fűrésztárcsa vastagságának (kerf) kezelése
- 📐 Dinamikus vágásoptimalizálás: legkevesebb rúd, minimális hulladék
- ♻️ Újrahasznosítható maradékanyagok figyelembevétele
- 🧠 Auditálható készletkezelés: raktárak és gépek ellenőrzése
- 🧩 Anyagcsoportonkénti és tárolónkénti korlátozások kezelése
- 🔄 Több raktár és gép párhuzamos kezelése (verziófüggő)
- 📦 Anyagcsoporton belüli többféle anyag támogatása (verziófüggő)
- 🌐 WordCode-alapú nyelvi illesztés előkészítve (verziófüggő)
- 📤 Exportálási lehetőségek: CSV, PDF, JSON (verziófüggő)
- 🧩 Bővíthető plugin-rendszer (verziófüggő)
- 🎨 Vizuálisan rendezett GUI Qt Designer 17.0 segítségével
- 🧱 MVP architektúra: jól elkülönített Model–View–Presenter rétegek

---

## 🛠 Fejlesztési eszközök

- Qt 6.9.2
- C++20
- Qt Designer 17.0
- qmake projektstruktúra

---

## ▶️ Futtatás

```bash
qmake && make ./CutPlanner
```

Vagy nyisd meg a projektet Qt Creatorban, és nyomd meg a 🟢 Run gombot.

---

## 📋 Használat

1. Add meg a darabigényt a bal oldali táblázatban (pl. 3000 mm × 4 db)
2. Állítsd be a fűrészlap vastagságát (alapértelmezés: 3 mm)
3. Kattints az „Optimize Plan” gombra
4. Nézd meg a vágási tervet, a hulladékot és a felhasználható maradékokat

---

## 🌱 Tervek a jövőre

- Rúd- és vágásvizualizáció (pl. színes sávok)
- Projekt mentés / betöltés
- Többféle rúd hossz támogatása
- Felhőalapú szinkronizálás
- Felhasználói jogosultságkezelés

---

## 👨‍💻 Fejlesztő

[Zoltán Horváth](https://www.linkedin.com/in/horvathzoltandebrecen/)

---

## 📄 Licenc

MIT License
