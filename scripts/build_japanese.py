"""Reproducible beginner deck + bitmap atlas; requires Pillow. UTF-8 throughout."""
import csv, json
from pathlib import Path
from PIL import Image, ImageFont, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'assets/japanese'
OUT = ROOT / 'src/folloup'
BASE = [
 ('あいうえお','a i u e o'), ('かきくけこ','ka ki ku ke ko'),
 ('さしすせそ','sa shi su se so'), ('たちつてと','ta chi tsu te to'),
 ('なにぬねの','na ni nu ne no'), ('はひふへほ','ha hi fu he ho'),
 ('まみむめも','ma mi mu me mo'), ('やゆよ','ya yu yo'),
 ('らりるれろ','ra ri ru re ro'), ('わをん','wa o n'),
 ('がぎぐげご','ga gi gu ge go'), ('ざじずぜぞ','za ji zu ze zo'),
 ('だぢづでど','da ji zu de do'), ('ばびぶべぼ','ba bi bu be bo'),
 ('ぱぴぷぺぽ','pa pi pu pe po'),
]
VOCAB = '''こんにちは|konnichiwa|hello
おはようございます|ohayou gozaimasu|good morning (polite)
こんばんは|konbanwa|good evening
おやすみなさい|oyasuminasai|good night
ありがとうございます|arigatou gozaimasu|thank you (polite)
すみません|sumimasen|excuse me; sorry
ごめんなさい|gomennasai|I am sorry
お願いします|onegaishimasu|please (request)
はい|hai|yes
いいえ|iie|no
はじめまして|hajimemashite|nice to meet you (first meeting)
よろしくお願いします|yoroshiku onegaishimasu|pleased to meet you; I look forward to working with you
お元気ですか|ogenki desu ka|how are you?
元気です|genki desu|I am well
私|watashi|I; me
あなた|anata|you
名前|namae|name
日本|nihon|Japan
日本語|nihongo|Japanese language
英語|eigo|English language
わかります|wakarimasu|I understand
わかりません|wakarimasen|I do not understand
もう一度|mou ichido|once more
ゆっくり|yukkuri|slowly
何|nani|what
どこ|doko|where
いつ|itsu|when
だれ|dare|who
いくら|ikura|how much
これ|kore|this one
それ|sore|that one (near you)
あれ|are|that one over there
ここ|koko|here
そこ|soko|there (near you)
水|mizu|water
お茶|ocha|tea
コーヒー|koohii|coffee
ご飯|gohan|rice; meal
パン|pan|bread
魚|sakana|fish
肉|niku|meat
野菜|yasai|vegetables
おいしい|oishii|delicious
ください|kudasai|please give me
食べます|tabemasu|eat (polite)
飲みます|nomimasu|drink (polite)
行きます|ikimasu|go (polite)
来ます|kimasu|come (polite)
見ます|mimasu|see; watch (polite)
話します|hanashimasu|speak (polite)
好き|suki|like; fond of
大丈夫|daijoubu|okay; all right
今日|kyou|today
明日|ashita|tomorrow
昨日|kinou|yesterday
今|ima|now
朝|asa|morning
昼|hiru|noon; daytime
夜|yoru|night
一|ichi|one
二|ni|two
三|san|three
四|yon|four
五|go|five
六|roku|six
七|nana|seven
八|hachi|eight
九|kyuu|nine
十|juu|ten
駅|eki|station
トイレ|toire|toilet; restroom
ホテル|hoteru|hotel
お店|omise|shop
友達|tomodachi|friend
家族|kazoku|family
子供|kodomo|child
妻|tsuma|my wife
夫|otto|my husband
医者|isha|doctor
歯医者|haisha|dentist
助けて|tasukete|help me
またね|mata ne|see you (casual)
さようなら|sayounara|goodbye'''

def main():
    cards = []
    for group in ('hiragana', 'katakana'):
        for row, readings in BASE:
            for char, reading in zip(row, readings.split()):
                jp = char if group == 'hiragana' else chr(ord(char) + 0x60)
                cards.append(dict(id=f'{group}-{ord(jp):x}', group=group,
                                  japanese=jp, romaji=reading, english=reading))
    for index, line in enumerate(VOCAB.splitlines()):
        jp, romaji, english = line.split('|')
        cards.append(dict(id=f'vocab-{index:03}', group='vocabulary',
                          japanese=jp, romaji=romaji, english=english))
    for group in ('hiragana', 'katakana'):
        for initial, reading in [('き','k'),('し','sh'),('ち','ch'),('に','n'),('ひ','h'),
                                 ('み','m'),('り','r'),('ぎ','g'),('じ','j'),('び','b'),('ぴ','p')]:
            for end, vowel in [('ゃ','a'),('ゅ','u'),('ょ','o')]:
                jp = initial + end
                if group == 'katakana': jp = ''.join(chr(ord(c)+0x60) for c in jp)
                cards.append(dict(id=f'{group}-{ord(jp[0]):x}-{ord(jp[1]):x}', group=group,
                                  japanese=jp, romaji=reading+'y'+vowel if reading not in ('sh','ch','j') else reading+vowel,
                                  english=reading+'y'+vowel if reading not in ('sh','ch','j') else reading+vowel))
    deck = dict(schema=1, name='Folloup One Conversational Japanese', cards=cards)
    dest = ROOT / 'sdcard/folloup/tutor'
    dest.mkdir(parents=True, exist_ok=True)
    (dest/'starter.json').write_text(json.dumps(deck,ensure_ascii=False,indent=2), encoding='utf-8')
    with (dest/'starter.csv').open('w',encoding='utf-8',newline='') as file:
        writer=csv.DictWriter(file,fieldnames=list(cards[0]))
        writer.writeheader(); writer.writerows(cards)
    # A compiled index bootstraps the SD deck if the user installs only firmware.
    with (OUT/'StarterDeck.h').open('w',encoding='utf-8') as file:
        file.write('#pragma once\n#include <cstddef>\nnamespace folloup {\nstruct Card {const char *id,*japanese,*romaji,*english; int group;};\ninline constexpr Card starter[] = {\n')
        for c in cards:
            values=[json.dumps(c[k],ensure_ascii=False) for k in ('id','japanese','romaji','english')]
            file.write('{'+','.join(values)+','+str(['hiragana','katakana','vocabulary'].index(c['group']))+'},\n')
        file.write('};\ninline constexpr size_t cardCount=sizeof(starter)/sizeof(starter[0]);\n}\n')
    chars = sorted(set(''.join(c['japanese'] for c in cards)))
    font=ImageFont.truetype(str(ASSETS/'NotoSansJP.ttf'),40)
    with (OUT/'JapaneseGlyphs.h').open('w',encoding='utf-8') as file:
        file.write('#pragma once\n#include <cstdint>\nnamespace folloup {\nstruct JapaneseGlyph {uint32_t cp; uint8_t bitmap[240];};\ninline constexpr JapaneseGlyph japaneseGlyphs[]={\n')
        for ch in chars:
            image=Image.new('1',(40,48),1)
            ImageDraw.Draw(image).text((0,-7),ch,font=font,fill=0)
            file.write('{'+str(ord(ch))+',{'+','.join(str(b^255) for b in image.tobytes())+'}},\n')
        file.write('};\n}\n')
    print(f'{len(cards)} cards, {len(chars)} glyphs; atlas {len(chars)*240} bytes')

if __name__=='__main__': main()
