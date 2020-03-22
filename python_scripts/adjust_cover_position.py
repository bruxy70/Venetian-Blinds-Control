"""
Adjust position only if the new position is different from current more than ADJUST_TRESHOLD
If the new position starts with '+', it will only increase the position
If the new position starts with '-', it will only decrease the position
"""
ADJUST_THRESHOLD = 5
CONF_POSITION='position'
CONF_MESSAGE='message'
CONF_ENTITY_ID='entity_id'
CONF_CURRENT_POSITION='current_position'

entity_id = data.get(CONF_ENTITY_ID)
position = data.get(CONF_POSITION)
message = data.get(CONF_MESSAGE,'')

ready=True
for parameter,parameter_name in [(entity_id,CONF_ENTITY_ID),(position,CONF_POSITION)]:
    if parameter is None:
        ready=False
        logger.error('adjust_cover_position.py: Missing {} data'.format(parameter_name))
if ready:
    try:
        current_position = hass.states.get(entity_id).attributes[CONF_CURRENT_POSITION]
    except:
        ready=False
        logger.error('adjust_cover_position.py: Cannot find {} attribute [{}]'.format(entity_id,CONF_CURRENT_POSITION))

if ready:
    logger.debug('adjust_cover_position.py: entity_id[{}], position[{}], message[{}]'.format(entity_id,position,message))

    if position[0]=='-':
        new_position=int(position[1:])
        if (new_position > current_position) and (abs(new_position - current_position)> ADJUST_THRESHOLD):
            hass.services.call('cover', 'set_cover_position', {
                'entity_id':entity_id,
                'position':new_position})
            hass.services.call('logbook','log',{
                'name':'{}'.format(entity_id),
                # 'entity_id':entity_id,
                # 'domain':'cover',
                'message':'has been set to position {} {}'.format(new_position,message)})
    elif position[0]=='+':
        new_position=int(position[1:])
        if (new_position < current_position) and (abs(new_position - current_position)> ADJUST_THRESHOLD):
            hass.services.call('cover', 'set_cover_position', {
                'entity_id':entity_id,
                'position':new_position})
            hass.services.call('logbook','log',{
                'name':'{}'.format(entity_id),
                # 'entity_id':entity_id,
                # 'domain':'cover',
                'message':'has been set to position {} {}'.format(new_position,message)})
    else:
        new_position=int(position)
        if abs(new_position - current_position)> ADJUST_THRESHOLD:B
            hass.services.call('cover', 'set_cover_position', {        
                'entity_id':entity_id,
                'position':new_position})
            hass.services.call('logbook','log',{
                'name':'{}'.format(entity_id),
                # 'entity_id':entity_id,
                # 'domain':'cover',
                'message':'has been set to position {} {}'.format(new_position,message)})
