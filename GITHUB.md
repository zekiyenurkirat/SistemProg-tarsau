# GitHub Depo Kurulumu

## Uzak Depo

🔗 https://github.com/zekiyenurkirat/SistemProg-tarsau

## Önerilen commit geçmişi

```bash
git init
git add include/ src/ Makefile README.md .gitignore tarsau.c
git commit -m "feat: modüler tarsau kaynak kodu ve Makefile"

git add tests/
git commit -m "test: örnek dosyalar ve otomatik test betiği"

git add rapor/
git commit -m "docs: akademik rapor ve test çıktıları"

git add GITHUB.md
git commit -m "docs: GitHub depo linki eklendi"
```

## Uzak depoya bağlama ve gönderme

```bash
git branch -M main
git remote add origin https://github.com/zekiyenurkirat/SistemProg-tarsau.git
git push -u origin main
```

## Teslim ZIP

```bash
zip -r ../SistemProg_Proje_teslim.zip . \
  -x "obj/*" "bin/tarsau" "tests/out/*" ".git/*" "a.sau"
```