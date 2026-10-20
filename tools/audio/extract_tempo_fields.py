#!/usr/bin/env python3
"""Extract completed immutable field observations from an existing per-hop trace."""
import argparse,json
from pathlib import Path
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('trace',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
 last=None;count=0
 with a.trace.open() as stream,a.output.open('w') as target:
  for line in stream:
   row=json.loads(line);field=row['tempo_field'];key=(field['epoch'],field['generation'])
   if field['generation'] and key!=last:
    observation={'pcm_sample_end':row['hop']*180,'media_end_48k':row['media_end_48k'],'result_available_us':row['available_us'],'presentation_us':row['available_us'],'field_publication_age_us':row['available_us']-field['available_us'],'tempo_field':field}
    target.write(json.dumps(observation,separators=(',',':'))+'\n');last=key;count+=1
 print(count,'completed field observations')
 return 0 if count else 1
if __name__=='__main__':raise SystemExit(main())
