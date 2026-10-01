ScriptName JM_EA_Native Hidden

Bool Function IsAvailable() Global Native

; Counts player-created / Soul Trap-filled instances of the supplied base soul-gem form.
; This intentionally reads ExtraSoul from inventory instances rather than only the base form.
Int Function CountSoulTrappedGems(Actor akOwner, SoulGem akBaseGem) Global Native

; Returns the strongest trapped soul present in the supplied base soul-gem form.
; 0=None, 1=Petty, 2=Lesser, 3=Common, 4=Greater, 5=Grand.
Int Function GetBestTrappedSoulLevel(Actor akOwner, SoulGem akBaseGem) Global Native

; Removes exactly one Soul Trap-filled inventory instance, preferring the strongest soul.
; Returns the soul level removed, or 0 if none was available.
Int Function ConsumeBestSoulTrappedGem(Actor akOwner, SoulGem akBaseGem) Global Native
