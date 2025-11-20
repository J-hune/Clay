# Clay

Projet utilisant **Qt**, **OpenGL**, et **Magnum**.

---

## Dépendances

### Outils requis

* CMake ≥ 3.16
* Compilateur C++17 (g++, clang++, ou MSVC)
* Git

### Bibliothèques système

* Qt ≥ 6.8
* OpenGL

### Installation des dépendances sous Ubuntu/Debian

```bash
sudo apt update
sudo apt install cmake g++ git
```

> Qt doit être installé manuellement depuis le site officiel : [https://www.qt.io/download](https://www.qt.io/download)

---

## Compilation
```bash
rm -rf build
cmake -S . -B build -G Ninja
ninja -C build
```

Ou
```bash
rm -rf build
cmake -S . -B build
cmake --build build
```

> `-S .` indique le dossier source, `-B build` indique le dossier où générer les fichiers temporaires.

---

## Exécution

```bash
./build/ClayApp
```

---

## Gestion de Qt6

Si vous obtenez une erreur de type :

```
CMake Error at CMakeLists.txt:... (find_package):
  Could not find a package configuration file provided by "Qt6" ...
```

cela signifie que CMake ne parvient pas à localiser Qt6.

### Solution

1. Assurez-vous que Qt6 est installé correctement sur votre système.
2. Définissez la variable d'environnement `CMAKE_PREFIX_PATH` pour indiquer le chemin où Qt6 est installé. Par exemple :

```bash
export CMAKE_PREFIX_PATH=/home/utilisateur/Qt/6.10.1/gcc_64/lib/cmake:$CMAKE_PREFIX_PATH
```