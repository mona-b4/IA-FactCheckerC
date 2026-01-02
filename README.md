# 🧠 IA-FactChecker

Projet IA fact-checker

Ce projet est une application en langage C qui utilise l’intelligence artificielle pour vérifier des affirmations formulées en langage naturel. Elle combine des techniques de recherche web, de traitement de texte, et d’analyse par IA pour produire des réponses argumentées et structurées.

## 🎯 Objectif

Permettre à un utilisateur de poser une question ou une affirmation (ex : *“La capitale de la France est Pékin ?”*) et d’obtenir une réponse automatique (VRAI / FAUX / PAS SÛR) accompagnée d’une explication et de sources, grâce à une IA.

## 🛠️ Technologies utilisées

- **Langage C** (compilation avec GCC)
- **API Mistral** pour l’analyse IA
- **DuckDuckGo + curl** pour la recherche web
- **cJSON** pour la manipulation de fichiers JSON
- **Win32 API** pour l’interface graphique (optionnelle)
- **MSYS2 / GitHub Desktop** pour le développement et la gestion de version

---

## ▶️ Instructions pour exécuter le code

### 1. Prérequis

- Système Windows avec terminal MSYS2 ou Git Bash
- `gcc` installé (via MinGW ou MSYS2)
- Connexion Internet
- Une clé API Mistral (à placer dans un fichier `.env` ou directement dans le code)

### 2. Compilation

Dans le terminal, placez-vous dans le dossier du projet et utilisez l’une des commandes suivantes selon votre besoin :

```bash
# Pour le mode terminal
gcc main.c -o agent

# Si vous avez des problèmes d'encodage
gcc main.c -o agent -finput-charset=UTF-8 -fexec-charset=UTF-8

# Pour compiler l'interface graphique (optionnelle)
gcc gui.c -o gui.exe -mwindows

### 3. Execution

  # En mode terminal ./agent  
  # En mode graphique ./gui.exe
