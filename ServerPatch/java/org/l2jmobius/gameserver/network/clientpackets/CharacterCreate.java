/*
 * Copyright (c) 2013 L2jMobius
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
package org.l2jmobius.gameserver.network.clientpackets;

import java.util.Collection;
import java.util.logging.Logger;

import org.l2jmobius.commons.util.StringUtil;
import org.l2jmobius.gameserver.config.PlayerConfig;
import org.l2jmobius.gameserver.config.ServerConfig;
import org.l2jmobius.gameserver.config.custom.AllowedPlayerRacesConfig;
import org.l2jmobius.gameserver.config.custom.FactionSystemConfig;
import org.l2jmobius.gameserver.config.custom.StartingLocationConfig;
import org.l2jmobius.gameserver.config.custom.StartingTitleConfig;
import org.l2jmobius.gameserver.data.sql.CharacterAppearanceTable;
import org.l2jmobius.gameserver.data.sql.PlayerInfoTable;
import org.l2jmobius.gameserver.data.xml.FakePlayerData;
import org.l2jmobius.gameserver.data.xml.InitialEquipmentData;
import org.l2jmobius.gameserver.data.xml.InitialShortcutData;
import org.l2jmobius.gameserver.data.xml.PlayerTemplateData;
import org.l2jmobius.gameserver.data.xml.SkillData;
import org.l2jmobius.gameserver.data.xml.SkillTreeData;
import org.l2jmobius.gameserver.entity.Location;
import org.l2jmobius.gameserver.entity.World;
import org.l2jmobius.gameserver.entity.actor.Player;
import org.l2jmobius.gameserver.entity.actor.appearance.PlayerAppearance;
import org.l2jmobius.gameserver.entity.actor.enums.player.PlayerClass;
import org.l2jmobius.gameserver.entity.actor.stat.PlayerStat;
import org.l2jmobius.gameserver.entity.actor.templates.PlayerTemplate;
import org.l2jmobius.gameserver.entity.item.enums.ItemProcessType;
import org.l2jmobius.gameserver.entity.item.holders.InitialEquipment;
import org.l2jmobius.gameserver.entity.item.instance.Item;
import org.l2jmobius.gameserver.mechanics.events.EventGroup;
import org.l2jmobius.gameserver.mechanics.events.EventHandler;
import org.l2jmobius.gameserver.mechanics.events.EventHandlers;
import org.l2jmobius.gameserver.mechanics.events.EventType;
import org.l2jmobius.gameserver.mechanics.events.holders.player.OnCharacterCreate;
import org.l2jmobius.gameserver.mechanics.skill.holders.SkillLearn;
import org.l2jmobius.gameserver.network.Disconnection;
import org.l2jmobius.gameserver.network.GameClient;
import org.l2jmobius.gameserver.network.PacketLogger;
import org.l2jmobius.gameserver.network.serverpackets.CharCreateFail;
import org.l2jmobius.gameserver.network.serverpackets.CharCreateOk;
import org.l2jmobius.gameserver.network.serverpackets.CharSelectionInfo;

/**
 * Handles the request from a client to create a new character.<br>
 * This packet processes the provided data to initialize a new {@code Player} in the world.
 * @author Mobius
 */
public class CharacterCreate extends ClientPacket
{
	protected static final Logger LOGGER_ACCOUNTING = Logger.getLogger("accounting");
	
	// cSdddddddddddd
	private String _name;
	private boolean _isFemale;
	private int _classId;
	private byte _hairStyle;
	private byte _hairColor;
	private byte _face;
	private byte[] _appearance;
	
	@Override
	protected void readImpl()
	{
		_name = readString();
		readInt(); // race
		_isFemale = readInt() != 0;
		_classId = readInt();
		readInt(); // _int
		readInt(); // _str
		readInt(); // _con
		readInt(); // _men
		readInt(); // _dex
		readInt(); // _wit
		_hairStyle = (byte) readInt();
		_hairColor = (byte) readInt();
		_face = (byte) readInt();
		
		// Alverion appearance extension: the client's appearance block, after its magic number.
		if ((remaining() >= 5) && (readInt() == CharacterAppearanceTable.MAGIC))
		{
			final int size = readUnsignedByte();
			if ((size > 0) && (size <= CharacterAppearanceTable.MAX_SIZE) && (remaining() >= size))
			{
				_appearance = readBytes(size);
			}
		}
	}
	
	@Override
	protected void runImpl()
	{
		final GameClient client = getClient();
		
		// Last Verified: May 30, 2009 - Gracia Final - Players are able to create characters with names consisting of as little as 1,2,3 letter/number combinations.
		if (_name.isEmpty() || (_name.length() > 16))
		{
			client.sendPacket(new CharCreateFail(CharCreateFail.REASON_16_ENG_CHARS));
			return;
		}
		
		if (PlayerConfig.FORBIDDEN_NAMES.length > 0)
		{
			for (String st : PlayerConfig.FORBIDDEN_NAMES)
			{
				if (_name.toLowerCase().contains(st.toLowerCase()))
				{
					client.sendPacket(new CharCreateFail(CharCreateFail.REASON_INCORRECT_NAME));
					return;
				}
			}
		}
		
		if (FakePlayerData.getInstance().getProperName(_name) != null)
		{
			client.sendPacket(new CharCreateFail(CharCreateFail.REASON_INCORRECT_NAME));
			return;
		}
		
		// Last Verified: May 30, 2009 - Gracia Final.
		if (!StringUtil.isAlphaNumeric(_name) || !isValidName(_name))
		{
			client.sendPacket(new CharCreateFail(CharCreateFail.REASON_INCORRECT_NAME));
			return;
		}
		
		if ((_face > 2) || (_face < 0))
		{
			PacketLogger.warning("Character Creation Failure: Character face " + _face + " is invalid. Possible client hack. " + client);
			client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
			return;
		}
		
		if ((_hairStyle < 0) || (!_isFemale && (_hairStyle > 4)) || (_isFemale && (_hairStyle > 6)))
		{
			PacketLogger.warning("Character Creation Failure: Character hair style " + _hairStyle + " is invalid. Possible client hack. " + client);
			client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
			return;
		}
		
		if ((_hairColor > 3) || (_hairColor < 0))
		{
			PacketLogger.warning("Character Creation Failure: Character hair color " + _hairColor + " is invalid. Possible client hack. " + client);
			client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
			return;
		}
		
		Player newChar = null;
		PlayerTemplate template = null;
		
		/*
		 * DrHouse: Since checks for duplicate names are done using SQL, lock must be held until data is written to DB as well.
		 */
		synchronized (PlayerInfoTable.getInstance())
		{
			final String accountName = client.getAccountName();
			if ((PlayerInfoTable.getInstance().getAccountCharacterCount(accountName) >= ServerConfig.MAX_CHARACTERS_NUMBER_PER_ACCOUNT) && (ServerConfig.MAX_CHARACTERS_NUMBER_PER_ACCOUNT != 0))
			{
				client.sendPacket(new CharCreateFail(CharCreateFail.REASON_TOO_MANY_CHARACTERS));
				return;
			}
			else if (PlayerInfoTable.getInstance().doesCharNameExist(_name))
			{
				client.sendPacket(new CharCreateFail(CharCreateFail.REASON_NAME_ALREADY_EXISTS));
				return;
			}
			
			template = PlayerTemplateData.getInstance().getTemplate(_classId);
			if ((template == null) || (PlayerClass.getPlayerClass(_classId).level() > 0))
			{
				client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
				return;
			}
			
			// Custom Feature: Disallow a race to be created.
			// Example: Humans can not be created if AllowHuman = False in Custom.properties.
			switch (template.getRace())
			{
				case HUMAN:
				{
					if (!AllowedPlayerRacesConfig.ALLOW_HUMAN)
					{
						client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
						return;
					}
					break;
				}
				case ELF:
				{
					if (!AllowedPlayerRacesConfig.ALLOW_ELF)
					{
						client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
						return;
					}
					break;
				}
				case DARK_ELF:
				{
					if (!AllowedPlayerRacesConfig.ALLOW_DARKELF)
					{
						client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
						return;
					}
					break;
				}
				case ORC:
				{
					if (!AllowedPlayerRacesConfig.ALLOW_ORC)
					{
						client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
						return;
					}
					break;
				}
				case DWARF:
				{
					if (!AllowedPlayerRacesConfig.ALLOW_DWARF)
					{
						client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
						return;
					}
					break;
				}
				case KAMAEL:
				{
					if (!AllowedPlayerRacesConfig.ALLOW_KAMAEL)
					{
						client.sendPacket(new CharCreateFail(CharCreateFail.REASON_CREATION_FAILED));
						return;
					}
					break;
				}
			}
			
			newChar = Player.create(template, accountName, _name, new PlayerAppearance(_face, _hairColor, _hairStyle, _isFemale));
		}
		
		// HP and MP are at maximum and CP is zero by default.
		newChar.setCurrentHp(newChar.getMaxHp());
		newChar.setCurrentMp(newChar.getMaxMp());
		// newChar.setMaxLoad(template.getBaseLoad());
		
		initNewChar(client, newChar);
		CharacterAppearanceTable.getInstance().store(newChar.getObjectId(), _appearance);
		client.sendPacket(CharCreateOk.STATIC_PACKET);
		
		LOGGER_ACCOUNTING.info("Created new character, " + newChar + ", " + client);
	}
	
	/**
	 * Checks if a name follows the allowed format.<br>
	 * It uses the pattern defined in {@code ServerConfig}.
	 * @param text The name string to validate.
	 * @return {@code true} if the name is valid, otherwise {@code false}.
	 */
	private boolean isValidName(String text)
	{
		return ServerConfig.CHARNAME_TEMPLATE_PATTERN.matcher(text).matches();
	}
	
	/**
	 * Initializes a newly created character in the game world.<br>
	 * This method sets up starting stats, items, skills, and locations.<br>
	 * It also handles initial equipment and notifies relevant systems.
	 * @param client The {@code GameClient} associated with the new character.
	 * @param newChar The {@code Player} object being initialized.
	 */
	private void initNewChar(GameClient client, Player newChar)
	{
		World.addObject(newChar);
		
		if (PlayerConfig.STARTING_ADENA > 0)
		{
			newChar.addAdena(ItemProcessType.REWARD, PlayerConfig.STARTING_ADENA, null, false);
		}
		
		final PlayerTemplate template = newChar.getTemplate();
		if (StartingLocationConfig.CUSTOM_STARTING_LOC)
		{
			final Location createLoc = new Location(StartingLocationConfig.CUSTOM_STARTING_LOC_X, StartingLocationConfig.CUSTOM_STARTING_LOC_Y, StartingLocationConfig.CUSTOM_STARTING_LOC_Z);
			newChar.setXYZInvisible(createLoc.getX(), createLoc.getY(), createLoc.getZ());
		}
		else if (FactionSystemConfig.FACTION_SYSTEM_ENABLED)
		{
			newChar.setXYZInvisible(FactionSystemConfig.FACTION_STARTING_LOCATION.getX(), FactionSystemConfig.FACTION_STARTING_LOCATION.getY(), FactionSystemConfig.FACTION_STARTING_LOCATION.getZ());
		}
		else
		{
			final Location createLoc = template.getCreationPoint();
			newChar.setXYZInvisible(createLoc.getX(), createLoc.getY(), createLoc.getZ());
		}
		
		newChar.setTitle(StartingTitleConfig.ENABLE_CUSTOM_STARTING_TITLE ? StartingTitleConfig.CUSTOM_STARTING_TITLE : "");
		
		if (PlayerConfig.ENABLE_VITALITY)
		{
			newChar.setVitalityPoints(Math.min(PlayerConfig.STARTING_VITALITY_POINTS, PlayerStat.MAX_VITALITY_POINTS), true);
		}
		
		if (PlayerConfig.STARTING_LEVEL > 1)
		{
			newChar.getStat().addLevel((byte) (PlayerConfig.STARTING_LEVEL - 1));
		}
		
		if (PlayerConfig.STARTING_SP > 0)
		{
			newChar.getStat().addSp(PlayerConfig.STARTING_SP);
		}
		
		final Collection<InitialEquipment> classEquipment = InitialEquipmentData.getInstance().getClassEquipment(newChar.getPlayerClass());
		if (classEquipment != null)
		{
			for (InitialEquipment equipment : classEquipment)
			{
				final Item item = newChar.getInventory().addItem(ItemProcessType.REWARD, equipment.getId(), equipment.getCount(), newChar, null);
				if (item == null)
				{
					PacketLogger.warning("Could not create item during player creation: itemId " + equipment.getId() + ", amount " + equipment.getCount() + ".");
					continue;
				}
				
				if (item.isEquipable() && equipment.isEquipped())
				{
					newChar.getInventory().equipItem(item);
				}
			}
		}
		
		for (SkillLearn skill : SkillTreeData.getInstance().getAvailableSkills(newChar, newChar.getPlayerClass(), false, true))
		{
			newChar.addSkill(SkillData.getInstance().getSkill(skill.getSkillId(), skill.getSkillLevel()), true);
		}
		
		// Register all shortcuts for actions, skills and items for this new character.
		InitialShortcutData.getInstance().registerAllShortcuts(newChar);
		
		final EventHandler players = EventGroup.PLAYERS.getHandler();
		if (EventHandlers.isRegistered(EventType.CHARACTER_CREATED, players))
		{
			EventHandlers.notify(new OnCharacterCreate(newChar, client), players);
		}
		
		newChar.setOnlineStatus(true, false);
		Disconnection.of(client, newChar).storeAndDelete();
		
		final CharSelectionInfo cl = new CharSelectionInfo(client.getAccountName(), client.getSessionId().playOkID1);
		client.setCharSelection(cl.getCharInfo());
	}
}
