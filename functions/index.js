// functions/index.js
require('dotenv').config();
const { onRequest } = require("firebase-functions/v2/https");
const crypto = require('crypto');
const line = require('@line/bot-sdk');
const admin = require('firebase-admin');

// Initialize Firebase Admin
admin.initializeApp();

const config = {
    channelAccessToken: process.env.CHANNEL_ACCESS_TOKEN,
    channelSecret: process.env.CHANNEL_SECRET
};

const client = new line.Client(config);

// สร้าง Flex Message สำหรับการเช็คชื่อ
function createAttendanceMessage(userName, userPicture, checkInTime, checkInDate) {
    return {
        type: "flex",
        altText: `บันทึกเวลาเข้างาน: ${userName}`,
        contents: {
            type: "bubble",
            body: {
                type: "box",
                layout: "vertical",
                contents: [
                    {
                        type: "box",
                        layout: "horizontal",
                        contents: [
                            {
                                type: "box",
                                layout: "vertical",
                                contents: [
                                    {
                                        type: "image",
                                        url: userPicture,
                                        aspectMode: "cover",
                                        size: "full"
                                    }
                                ],
                                cornerRadius: "100px",
                                width: "72px",
                                height: "72px"
                            },
                            {
                                type: "box",
                                layout: "vertical",
                                contents: [
                                    {
                                        type: "text",
                                        contents: [
                                            {
                                                type: "span",
                                                text: userName,
                                                weight: "bold",
                                                color: "#000000"
                                            },
                                            {
                                                type: "span",
                                                text: "     "
                                            },
                                            {
                                                type: "span",
                                                text: "บันทึกเวลาเข้างานเรียบร้อย ✅"
                                            }
                                        ],
                                        size: "sm",
                                        wrap: true
                                    },
                                    {
                                        type: "box",
                                        layout: "baseline",
                                        contents: [
                                            {
                                                type: "text",
                                                text: `${checkInTime} น.`,
                                                size: "xl",
                                                color: "#27ACB2",
                                                weight: "bold"
                                            }
                                        ],
                                        spacing: "sm",
                                        margin: "md"
                                    },
                                    {
                                        type: "text",
                                        text: checkInDate,
                                        size: "sm",
                                        color: "#bcbcbc"
                                    }
                                ]
                            }
                        ],
                        spacing: "xl",
                        paddingAll: "20px"
                    }
                ],
                paddingAll: "0px"
            }
        }
    };
}

// เช็คว่าวันนี้ได้เช็คชื่อไปแล้วหรือยัง
async function hasCheckedInToday(userId) {
    const today = new Date();
    today.setHours(0, 0, 0, 0);
    
    const snapshot = await admin.firestore()
        .collection('attendance')
        .where('userId', '==', userId)
        .where('timestamp', '>=', today)
        .limit(1)
        .get();
    
    return !snapshot.empty;
}

// บันทึกการเช็คชื่อ
async function recordAttendance(userId, userName, timestamp) {
    await admin.firestore().collection('attendance').add({
        userId,
        userName,
        timestamp,
        createdAt: admin.firestore.FieldValue.serverTimestamp()
    });
}

exports.lineWebhook = onRequest({
    region: 'asia-northeast1'
}, async (req, res) => {
    console.log('Webhook called');
    
    const signature = req.headers['x-line-signature'];
    if (!signature || !verifySignature(req.body, signature)) {
        console.error('Invalid signature');
        res.status(403).send('Invalid signature');
        return;
    }

    try {
        const events = req.body.events;
        console.log('Received events:', JSON.stringify(events));

        await Promise.all(events.map(async (event) => {
            if (event.type === 'beacon' && event.beacon.type === 'enter') {
                const userId = event.source.userId;
                
                try {
                    // เช็คว่าเช็คชื่อวันนี้ไปแล้วหรือยัง
                    const alreadyCheckedIn = await hasCheckedInToday(userId);
                    if (alreadyCheckedIn) {
                        return client.replyMessage(event.replyToken, {
                            type: 'text',
                            text: 'คุณได้เช็คชื่อไปแล้วในวันนี้'
                        });
                    }

                    // ดึงข้อมูลผู้ใช้
                    const profile = await client.getProfile(userId);
                    const now = new Date();
                    
                    // สร้างข้อความวันที่และเวลาแบบไทย
                    const checkInTime = now.toLocaleTimeString('th-TH', {
                        timeZone: 'Asia/Bangkok',
                        hour: '2-digit',
                        minute: '2-digit',
                        hour12: false
                    });

                    const checkInDate = now.toLocaleDateString('th-TH', {
                        timeZone: 'Asia/Bangkok',
                        year: 'numeric',
                        month: 'long',
                        day: 'numeric'
                    });

                    // บันทึกการเช็คชื่อ
                    await recordAttendance(userId, profile.displayName, now);

                    // ส่งข้อความยืนยันการเช็คชื่อ
                    const message = createAttendanceMessage(
                        profile.displayName,
                        profile.pictureUrl,
                        checkInTime,
                        checkInDate
                    );

                    return client.replyMessage(event.replyToken, message);
                } catch (error) {
                    console.error('Error processing attendance:', error);
                    const fallbackMessage = {
                        type: 'text',
                        text: 'ขออภัย เกิดข้อผิดพลาดในการบันทึกเวลา กรุณาลองใหม่อีกครั้ง'
                    };
                    return client.replyMessage(event.replyToken, fallbackMessage);
                }
            }
        }));

        res.status(200).send('OK');
    } catch (error) {
        console.error('Error processing webhook:', error);
        res.status(500).send('Internal Server Error');
    }
});