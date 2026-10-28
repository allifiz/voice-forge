# VoiceForge Lite

VoiceForge Lite adalah **prototipe** voice changer real-time berbasis C++ untuk laptop Windows low-end. Pemrosesan berjalan di CPU, tanpa GPU NVIDIA, Python, Electron, atau model AI. Audio dari mikrofon diproses dengan **pitch shifting dan formant shifting**, lalu dikirim ke speaker/virtual cable yang dipilih.

> **Ekspektasi realistis:** aplikasi ini bukan AI female-voice converter. Suara perempuan yang benar-benar natural **belum dijamin**. Hasil perlu diuji dengan suara manusia, bukan hanya angka benchmark. Ini versi CLI (terminal), belum ada GUI.

## Kebutuhan

- Windows 10/11 x64
- Visual Studio 2022 Build Tools dengan **Desktop development with C++**, MSVC v143 dan Windows SDK
- CMake (dapat disertakan oleh Visual Studio) dan Git for Windows
- Koneksi internet **saat konfigurasi pertama**, untuk mengunduh library C++
- Opsional: [VB-CABLE](https://vb-audio.com/Cable/) untuk mengirim hasil ke Discord/OBS

Komponen audio open source yang diambil otomatis saat konfigurasi:
- [miniaudio](https://github.com/mackron/miniaudio) untuk device input/output
- [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) untuk pemrosesan pitch/formant
- [Signalsmith Linear](https://github.com/Signalsmith-Audio/linear) untuk header STFT yang dibutuhkan Stretch

Library pihak ketiga tidak disalin ke repository; tinjau masing-masing lisensi sebelum mendistribusikan aplikasi jadi.

## 1. Clone (pertama kali)

Buka **x64 Native Tools Command Prompt for VS 2022**, atau PowerShell yang sudah mengenali CMake dan compiler Visual Studio:

~~~powershell
cd $HOME\Downloads
git clone https://github.com/allifiz/voice-forge.git
cd voice-forge
cmake --version
git --version
~~~

Bila sudah clone sebelumnya, **jangan clone ulang**: masuk folder repository lalu jalankan \`git pull origin main\`.

**Penting:** folder ZIP lama (\`voiceforge-lite-v0.1-source\`) bukan clone Git repository ini. Untuk pertama kali, clone ke folder baru agar riwayat Git bersih.

## 2. Build Windows

~~~powershell
.\scripts\build-windows.bat
~~~

Jika berhasil, hasilnya:

~~~text
build\Release\voiceforge.exe
~~~

Tes executable:

~~~powershell
.\build\Release\voiceforge.exe --help
.\build\Release\voiceforge.exe --list-devices
~~~

Jika script melaporkan CMake tidak ditemukan padahal sudah terinstal, buka terminal baru atau gunakan **x64 Native Tools Command Prompt for VS 2022**.

> Peringatan \`FetchContent_Populate is deprecated\`, \`C4244\` dari miniaudio, atau \`C4305\` dari Signalsmith tidak otomatis berarti build gagal. Cari baris \`error C...\` atau \`FAILED\` untuk penyebab sebenarnya.

## 3. Tes kemampuan laptop tanpa mikrofon

~~~powershell
.\build\Release\voiceforge.exe --benchmark
~~~

Benchmark ini memproses 10 detik sinyal sintetis dengan blok 10 ms dan menampilkan waktu komputasi rata-rata/maksimum. **Bukan** ukuran latensi mikrofon-ke-Discord ataupun bukti kualitas natural. Bila waktu proses terus melebihi 10 ms per blok, ada risiko suara tersendat.

## 4. Hubungkan dengan Discord melalui VB-CABLE

1. Install [VB-CABLE](https://vb-audio.com/Cable/) dan restart Windows bila diminta.
2. Jalankan:
   
   ~~~powershell
   .\build\Release\voiceforge.exe --list-devices
   ~~~

3. Dari daftar **INPUT DEVICES**, catat angka mikrofon asli. Dari **OUTPUT DEVICES**, catat angka **CABLE Input (VB-Audio Virtual Cable)**.
4. Jalankan program menggunakan angka **sesuai laptopmu**:

   ~~~powershell
   .\build\Release\voiceforge.exe --input 0 --output 4 --pitch 5 --formant 1.16 --gain 0.85
   ~~~

   **0 dan 4 hanya contoh indeks**, jangan disalin tanpa memeriksa daftar.
5. Discord > **User Settings** > **Voice & Video** > **Input Device**: pilih **CABLE Output (VB-Audio Virtual Cable)**.
6. Gunakan fitur tes mikrofon Discord. **Pakai headset** agar audio tidak feedback. Terminal harus tetap terbuka selama voice changer dipakai. Tekan Enter untuk menghentikan.

VB-CABLE memakai nama yang terkesan terbalik: **CABLE Input** adalah tujuan playback dari VoiceForge, sedangkan **CABLE Output** adalah sumber recording bagi Discord.

Untuk testing sebelum mengatur Discord, boleh memilih output berupa headphone; jangan gunakan speaker eksternal saat mikrofon aktif karena feedback.

## Preset awal

| Karakter suara | \`--pitch\` | \`--formant\` |
| --- | ---: | ---: |
| Subtle | 3 | 1.08 |
| Feminine experiment | 5 | 1.16 |
| High | 7 | 1.22 |

Semua preset **eksperimental**. Pitch tinggi dapat memperburuk artefak, dan naturalness dipengaruhi artikulasi suara asli.

## Troubleshooting

**\`cmake\` tidak dikenali:** install CMake atau perbaiki PATH, lalu restart terminal.

**\`signalsmith-linear/stft.h\` missing:** pastikan \`CMakeLists.txt\` versi repository ini sudah dipakai dan konfigurasi online berhasil. Bila build cache dari ZIP lama, gunakan folder clone bersih.

**\`error C2589\` atau \`C2059\` pada \`std::min/max\`:** macro \`min/max\` Windows bentrok; repository sudah mendefinisikan \`NOMINMAX\` di CMake dan source. Pastikan branch \`main\` paling baru.

**Tidak ada suara:** cek angka input dan output melalui \`--list-devices\`, perizinan mikrofon Windows, Discord Input Device, dan apakah terminal VoiceForge masih berjalan.

**Crackle/lag:** tutup program lain yang berat, tes \`--benchmark\`, coba pitch lebih rendah, dan periksa sample-rate audio device. Blok DSP 10 ms **tidak berarti total latency hanya 10 ms**.

## Status dan arah pengembangan

- **v0.1.2:** CLI dengan input/output pilihan, realtime pitch/formant, gain, benchmark, dependensi Signalsmith Linear, perbaikan \`NOMINMAX\`.
- Belum diuji build dan kualitas di laptop Windows pengguna. Status berhasil hanya boleh dinyatakan setelah dikompilasi dan dicek di mesin target.
- Berikutnya: GUI ringan, preview suara, pengaturan preset secara langsung, meter audio, bypass, device hotplug, pengukuran latensi nyata dan uji DSP alternatif untuk naturalness.

Tidak ada workflow GitHub Actions dalam repository ini agar build/testing tetap lokal dan tidak memicu biaya CI tanpa kebutuhan.
