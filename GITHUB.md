# GitHub Depo Kurulumu

## Önerilen commit geçmişi

```bash
git init
git add include/ src/ Makefile README.md .gitignore
git commit -m "feat: modüler tarsau kaynak kodu ve Makefile"

git add tests/
git commit -m "test: örnek dosyalar ve otomatik test betiği"

git add rapor/
git commit -m "docs: akademik rapor ve test çıktıları"
```

## Uzak depo

```bash
git branch -M main
git remote add origin https://github.com/KULLANICI_ADINIZ/SistemProg-tarsau.git
git push -u origin main
```

## Teslim ZIP

```bash
zip -r ../SistemProg_Proje_teslim.zip . \
  -x "obj/*" "bin/tarsau" "tests/out/*" ".git/*" "a.sau"
```
