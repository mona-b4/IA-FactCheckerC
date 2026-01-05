# 🧠 IA-FactChecker

Projet IA fact-checker

Ce projet est une application en langage C qui utilise l’intelligence artificielle pour vérifier des affirmations ou questions formulées en langage naturel. Elle combine des techniques de recherche web, de traitement de texte, et d’analyse par IA pour produire des réponses argumentées et structurées.

## 🎯 Objectif

Permettre à un utilisateur de poser une question ou une affirmation (ex : *“La capitale de la France est Pékin ?”*) et d’obtenir une réponse automatique verifiée.

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
- Python 3.10+ installé
- Connexion Internet
- Une clé API Mistral (à placer dans un fichier `.env` ou directement dans le code)

### 2. Compilation

Dans le terminal, placez-vous dans le dossier du projet et utilisez l’une des commandes suivantes :

```bash
# Compilation standard
gcc main.c cJSON.c -o agent.exe

# Si vous avez des problèmes d'encodage
gcc main.c cJSON.c -o agent.exe -finput-charset=UTF-8 -fexec-charset=UTF-8

# Puis placez-vous dans le dossier web_interface/ puis installez les dépendances :
cd web_interface
pip install -r requirements.txt

# Lancez le serveur Flask :
python app.py

Puis ouvrez votre navigateur à l’adresse :
👉 http://127.0.0.1:5000

### 5. Utilisation
Entrez une question dans le champ prévu

- Cliquez sur “Vérifier
- L’IA vous renverra :
- Une réponse (VRAI ou FAUX)
- Une explication
- Un niveau de confiance
- Des sources
