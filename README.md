# Line Beacon Project

This repository contains the source code for the Line Beacon Project, which integrates an ESP32 microcontroller with LINE's Beacon functionality to track and manage attendance. The project also uses Firebase for backend services and LINE Flex Messages for communication.

โครงการนี้เป็นโครงการที่ใช้ ESP32 ร่วมกับฟังก์ชัน Beacon ของ LINE เพื่อจัดการและติดตามการเข้างาน โครงการยังใช้ Firebase สำหรับบริการ Backend และใช้ LINE Flex Messages สำหรับการสื่อสาร

---

## Project Structure

```
.
├── ESP32 WROOM
│   └── sketch_jan22a.ino
├── firebase.json
└── functions
    ├── index.js
    ├── package-lock.json
    └── package.json
```

### ESP32 WROOM
- **`sketch_jan22a.ino`**: Contains the ESP32 firmware code for handling Bluetooth communication and beacon broadcasting.
  - ไฟล์นี้มีโค้ดเฟิร์มแวร์ของ ESP32 ที่จัดการการสื่อสาร Bluetooth และการส่งสัญญาณ Beacon

### Firebase Functions
- **`index.js`**: Implements the backend logic for handling LINE Webhook events, including attendance tracking and messaging.
  - ไฟล์นี้ใช้สำหรับจัดการ Webhook ของ LINE และติดตามการเข้างานพร้อมทั้งส่งข้อความ
- **`package.json`**: Defines dependencies for the Firebase Functions.
  - ไฟล์ที่ระบุ Dependencies ของ Firebase Functions
- **`package-lock.json`**: Lock file for dependency versions.
  - ไฟล์ล็อกสำหรับระบุเวอร์ชัน Dependencies

### `firebase.json`
- Configuration file for Firebase, defining hosting and function settings.
  - ไฟล์กำหนดค่า Firebase ที่ใช้กำหนดการตั้งค่า Hosting และ Function

---

## Features

### ESP32 Firmware
- Broadcasts a BLE (Bluetooth Low Energy) beacon signal with custom advertising parameters.
  - ส่งสัญญาณ Beacon แบบ BLE พร้อมการตั้งค่าการโฆษณาแบบกำหนดเอง
- Supports LINE’s UUID for Beacon communication.
  - รองรับ UUID ของ LINE สำหรับการสื่อสาร Beacon
- Handles the setup and initialization of BLE advertising.
  - จัดการการตั้งค่าและเริ่มต้นการโฆษณา BLE
- Sends device messages for attendance tracking.
  - ส่งข้อความจากอุปกรณ์สำหรับติดตามการเข้างาน

### Firebase Functions
- Processes LINE Webhook events for beacon entry detections.
  - จัดการ Webhook ของ LINE สำหรับการตรวจจับการเข้า Beacon
- Checks if a user has already checked in for the day.
  - ตรวจสอบว่าผู้ใช้ได้เช็คชื่อในวันนั้นหรือยัง
- Stores attendance records in Firebase Firestore.
  - บันทึกข้อมูลการเข้างานใน Firebase Firestore
- Sends customized LINE Flex Messages to users upon successful check-in.
  - ส่ง LINE Flex Message แบบกำหนดเองให้ผู้ใช้เมื่อเช็คชื่อสำเร็จ

### LINE Flex Message
The Flex Message displays:
1. User's name
2. User's profile picture
3. Check-in time
4. Check-in date

Flex Message จะแสดง:
1. ชื่อของผู้ใช้
2. รูปโปรไฟล์ของผู้ใช้
3. เวลาที่เช็คชื่อ
4. วันที่เช็คชื่อ

Example Flex Message:
```json
{
  "type": "flex",
  "altText": "บันทึกเวลาเข้างาน: [User Name]",
  "contents": {
    "type": "bubble",
    "body": {
      "type": "box",
      "layout": "vertical",
      "contents": [
        {
          "type": "box",
          "layout": "horizontal",
          "contents": [
            {
              "type": "box",
              "layout": "vertical",
              "contents": [
                {
                  "type": "image",
                  "url": "[User Picture URL]",
                  "aspectMode": "cover",
                  "size": "full"
                }
              ],
              "cornerRadius": "100px",
              "width": "72px",
              "height": "72px"
            },
            {
              "type": "box",
              "layout": "vertical",
              "contents": [
                {
                  "type": "text",
                  "contents": [
                    {
                      "type": "span",
                      "text": "[User Name]",
                      "weight": "bold",
                      "color": "#000000"
                    },
                    {
                      "type": "span",
                      "text": "     "
                    },
                    {
                      "type": "span",
                      "text": "บันทึกเวลาเข้างานเรียบร้อย ✅"
                    }
                  ],
                  "size": "sm",
                  "wrap": true
                },
                {
                  "type": "box",
                  "layout": "baseline",
                  "contents": [
                    {
                      "type": "text",
                      "text": "[Check-in Time] น.",
                      "size": "xl",
                      "color": "#27ACB2",
                      "weight": "bold"
                    }
                  ],
                  "spacing": "sm",
                  "margin": "md"
                },
                {
                  "type": "text",
                  "text": "[Check-in Date]",
                  "size": "sm",
                  "color": "#bcbcbc"
                }
              ]
            }
          ],
          "spacing": "xl",
          "paddingAll": "20px"
        }
      ],
      "paddingAll": "0px"
    }
  }
}
```

---

## Prerequisites

### Hardware
- ESP32 WROOM Development Board
  - บอร์ดพัฒนา ESP32 WROOM

### Software
- Arduino IDE
- Firebase CLI
- Node.js and npm
- LINE Developers account
  - บัญชีผู้พัฒนาของ LINE

### Environment Variables
Create a `.env` file in the `functions` directory with the following keys:
สร้างไฟล์ `.env` ในไดเรกทอรี `functions` โดยเพิ่มคีย์ดังนี้:
```env
CHANNEL_ACCESS_TOKEN=your-line-channel-access-token
CHANNEL_SECRET=your-line-channel-secret
```

---

## How to Deploy

1. **Set up Firebase CLI:**
   ตั้งค่า Firebase CLI:
   ```bash
   firebase login
   firebase init
   ```
2. **Install Dependencies:**
   ติดตั้ง Dependencies:
   ```bash
   cd functions
   npm install
   ```
3. **Deploy Functions:**
   อัปโหลด Functions ไปยัง Firebase:
   ```bash
   firebase deploy
   ```

---

## How to Use

1. Flash the ESP32 with the firmware provided in `ESP32 WROOM/sketch_jan22a.ino`.
   - อัปโหลดเฟิร์มแวร์ลงใน ESP32 จากไฟล์ `ESP32 WROOM/sketch_jan22a.ino`
2. Configure your LINE Beacon settings in the LINE Developers Console.
   - ตั้งค่าการใช้งาน Beacon ใน LINE Developers Console
3. Deploy the Firebase functions and ensure the webhook URL is correctly set in LINE Developers Console.
   - อัปโหลด Firebase Functions และตรวจสอบว่า Webhook URL ถูกตั้งค่าใน LINE Developers Console
4. Users will receive a Flex Message upon entering the beacon range.
   - ผู้ใช้จะได้รับ Flex Message เมื่อเข้าสู่ระยะสัญญาณ Beacon

---

## License
This project is licensed under the MIT License.
โครงการนี้ใช้สัญญาอนุญาตแบบ MIT License

