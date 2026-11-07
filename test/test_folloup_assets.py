import json, unittest, struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

class Assets(unittest.TestCase):
    def test_deck(self):
        deck=json.loads((ROOT/'sdcard/folloup/tutor/starter.json').read_text(encoding='utf-8'))
        self.assertEqual(deck['schema'],1)
        cards=deck['cards']
        self.assertEqual(len(cards),291)
        self.assertEqual(len({c['id'] for c in cards}),len(cards))
        self.assertLessEqual(len(cards),512)
        self.assertEqual({c['group'] for c in cards},{'hiragana','katakana','vocabulary'})
        self.assertTrue(all(c[k] for c in cards for k in ('id','japanese','english','romaji')))
        glyphs=(ROOT/'src/folloup/JapaneseGlyphs.h').read_text(encoding='utf-8')
        for c in set(''.join(c['japanese'] for c in cards)):
            self.assertIn('{'+str(ord(c))+',{',glyphs)

    def test_partitions(self):
        previous=0
        for raw in (ROOT/'partitions.csv').read_text().splitlines():
            if not raw.strip() or raw.startswith('#'):continue
            name,kind,subtype,offset,size,*_=map(str.strip,raw.split(','))
            offset,size=int(offset,0),int(size,0)
            self.assertGreaterEqual(offset,previous)
            if kind=='app':self.assertEqual(offset%0x10000,0)
            previous=offset+size
            self.assertLessEqual(previous,16*1024*1024)
        self.assertEqual(previous,16*1024*1024)

if __name__=='__main__':unittest.main()
