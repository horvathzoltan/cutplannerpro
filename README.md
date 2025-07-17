## CutPlanner MVP
🪚 Qt Widgets-alapú anyagoptimalizáló alkalmazás, amely MVP architektúrát követ. Segítségével különböző hosszúságú darabokat vághatsz le 6 méteres rudakból minimális veszteséggel, figyelembe véve a fűrésztárcsa szélességét és újrahasználható maradékokat is.

## 🚀 Fő funkciók
- Darabigények megadása: hossz és darabszám szerint
- Fűrésztárcsa vastagságának (kerf) kezelése
- Dinamikus vágásoptimalizálás: legkevesebb rúd, minimális hulladék
- Újrahasznosítható maradékanyagok figyelembevétele
- Vizuálisan rendezett GUI Qt Designer segítségével
- MVP architektúra: jól elkülönített Model–View–Presenter rétegek

## 🛠 Fejlesztési eszközök
- Qt 6.x / Qt 5.x
- C++17
- Qt Designer
- (opcionálisan) CMake vagy Qt Creator build-rendszer

## 🧱 Projektstruktúra
```text 
CutPlanner
├── model/
│ └── CuttingOptimizerModel.h/.cpp
├── presenter/
│ └── CuttingPresenter.h/.cpp
├── view/
│ └── MainWindow.ui
│ └── MainWindow.h/.cpp
├── main.cpp
└── README.md
```



## ▶️ Futtatás
```bash
qmake && make ./CutPlanner
```

Vagy nyisd meg a projektet Qt Creatorban, és nyomd meg a 🟢 Run gombot.

## 📋 Használat
1. Add meg a darabigényt a bal oldali táblázatban (pl. 3000 mm × 4 db)
2. Állítsd be a fűrészlap vastagságát (alapértelmezés: 3 mm)
3. Kattints az Optimize Plan gombra
4. Nézd meg a vágási tervet és a felhasználható maradékokat


## 🌱 Tervek a jövőre
CSV exportálás a vágási tervhez
Rúd- és vágásvizualizáció (pl. színes sávok)
Projekt mentés / betöltés
Többféle rúd hossz támogatása

## 👨‍💻 Fejlesztő
https://www.linkedin.com/in/horvathzoltandebrecen/

## 📄 Licenc
MIT License
