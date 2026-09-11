"""Small original procedural sounds. No third-party audio or codec dependency."""
from pathlib import Path
import array
import math
import random
import wave
import unreal
from import_demo_art import import_file, DEST


def main():
    root=Path(unreal.Paths.project_dir())/'SourceArt/Original/Audio'
    root.mkdir(parents=True,exist_ok=True)
    rate=22050
    sounds={'Cast':(.35,[320,480]),'Recover':(.55,[523.25,659.25,783.99]),'Build':(.85,[392,523.25,659.25]),'Signal':(2.4,[392,523.25,659.25,783.99,1046.5]),'Plant':(.25,[440,587.33])}
    for name,(duration,notes) in sounds.items():
        samples=array.array('h')
        for i in range(int(duration*rate)):
            t=i/rate
            value=0
            for j,freq in enumerate(notes):
                age=t-j*duration/(len(notes)*2)
                if age>=0:
                    value += math.sin(math.tau*freq*age)*math.exp(-age*9)*min(1,age*100)*.19
            samples.append(int(max(-1,min(1,value))*32767))
        path=root/(name+'.wav')
        with wave.open(str(path),'wb') as output:
            output.setnchannels(1); output.setsampwidth(2); output.setframerate(rate); output.writeframes(samples.tobytes())
        sound=import_file(path,'A_'+name)
        sound.set_editor_property('volume',.55)
        unreal.EditorAssetLibrary.save_loaded_asset(sound)
    unreal.EditorAssetLibrary.save_directory(DEST,only_if_is_dirty=True,recursive=True)


if __name__=='__main__': main()
