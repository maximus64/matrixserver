#ifndef SOUNDEFFECT_H_
#define SOUNDEFFECT_H_


class SoundEffect{
public:
  enum Sound { charging, select };
  SoundEffect();
  void playSound(Sound sid);
};


#endif /* SOUNDEFFECT_H_ */
